// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_PATH_TABLE_H
#define PXR_SDF_PATH_TABLE_H

#include "./api.h"
#include "./path.h"
#include <pxr/tf/pointerAndBits.h>
#include <pxr/tf/functionRef.h>

#include <algorithm>
#include <utility>
#include <vector>

namespace pxr {

// Parallel visitation helper function.
SDF_API
void Sdf_VisitPathTableInParallel(void **, size_t, TfFunctionRef<void(void*&)>);

/// \class SdfPathTable
///
/// A mapping from SdfPath to \a MappedType, somewhat similar to map<SdfPath,
/// MappedType> and TfHashMap<SdfPath, MappedType>, but with key
/// differences.  Notably:
///
/// Works exclusively with absolute paths.
///
/// Inserting a path \a p also implicitly inserts all of \a p's ancestors.
///
/// Erasing a path \a p also implicitly erases all of \a p's descendants.
///
/// The table has an order: it's a preordering of the paths in the table, but
/// with arbitrary sibling order.  Given a path \a p in the table, all other
/// paths in the table with \a p as a prefix appear contiguously, immediately
/// following \a p.  For example, suppose a table contains the paths:
///
///   {'/a/b/c', '/a', '/a/d', '/', '/a/b'}
///
/// Then there are two possible valid orderings:
///
///   ['/', '/a', '/a/d', '/a/b', '/a/b/c']
///   ['/', '/a', '/a/b', '/a/b/c', '/a/d']
///
/// In addition to the ordinary map and TfHashMap methods, this class
/// provides a method \a FindSubtreeRange, which, given a path \a p, returns
/// a pair of iterators [\a b, \a e) defining a range such that for every
/// iterator \a i in [\a b, \a e), i->first is either equal to \a p or is
/// prefixed by \a p.
///
/// Iterator Invalidation
///
/// Like most other node-based containers, iterators are only invalidated when
/// the element they refer to is removed from the table.  Note however, that
/// since removing the element with path \a p also implicitly removes all
/// elements with paths prefixed by \a p, a call to erase(\a i) may invalidate
/// many iterators.
///
template <class MappedType>
class SdfPathTable
{
public:

    typedef SdfPath key_type;
    typedef MappedType mapped_type;
    typedef std::pair<key_type, mapped_type> value_type;

private:

    // An _Entry represents an item in the table.  It holds the item's value, a
    // pointer (\a next) to the next item in the hash bucket's linked list, and
    // two pointers (\a firstChild and \a nextSibling) that describe the tree
    // structure.
    struct _Entry {
        _Entry(const _Entry&) = delete;
        _Entry& operator=(const _Entry&) = delete;
        _Entry(value_type const &value, _Entry *n)
            : value(value)
            , next(n)
            , firstChild(nullptr)
            , nextSiblingOrParent(nullptr, false) {}

        _Entry(value_type &&value, _Entry *n)
            : value(std::move(value))
            , next(n)
            , firstChild(nullptr)
            , nextSiblingOrParent(nullptr, false) {}

        // If this entry's nextSiblingOrParent field points to a sibling, return
        // a pointer to it, otherwise return null.
        _Entry *GetNextSibling() {
            return nextSiblingOrParent.template BitsAs<bool>() ?
                nextSiblingOrParent.Get() : 0;
        }
        // If this entry's nextSiblingOrParent field points to a sibling, return
        // a pointer to it, otherwise return null.
        _Entry const *GetNextSibling() const {
            return nextSiblingOrParent.template BitsAs<bool>() ?
                nextSiblingOrParent.Get() : 0;
        }

        // If this entry's nextSiblingOrParent field points to a parent, return
        // a pointer to it, otherwise return null.
        _Entry *GetParentLink() {
            return nextSiblingOrParent.template BitsAs<bool>() ? 0 :
                nextSiblingOrParent.Get();
        }
        // If this entry's nextSiblingOrParent field points to a parent, return
        // a pointer to it, otherwise return null.
        _Entry const *GetParentLink() const {
            return nextSiblingOrParent.template BitsAs<bool>() ? 0 :
                nextSiblingOrParent.Get();
        }

        // Set this entry's nextSiblingOrParent field to point to the passed
        // sibling.
        void SetSibling(_Entry *sibling) {
            nextSiblingOrParent.Set(sibling, /* isSibling */ true);
        }

        // Set this entry's nextSiblingOrParent field to point to the passed
        // parent.
        void SetParentLink(_Entry *parent) {
            nextSiblingOrParent.Set(parent, /* isSibling */ false);
        }

        // Add \a child as a child of this entry.
        void AddChild(_Entry *child) {
            // If there are already one or more children, make \a child be our
            // new first child.  Otherwise, add \a child as the first child.
            if (firstChild)
                child->SetSibling(firstChild);
            else
                child->SetParentLink(this);
            firstChild = child;
        }

        void RemoveChild(_Entry *child) {
            // Remove child from this _Entry's children.
            if (child == firstChild) {
                firstChild = child->GetNextSibling();
            } else {
                // Search the list to find the preceding child, then unlink the
                // child to remove.
                _Entry *prev, *cur = firstChild;
                do {
                    prev = cur;
                    cur = prev->GetNextSibling();
                } while (cur != child);
                prev->nextSiblingOrParent = cur->nextSiblingOrParent;
            }
        }

        // The value object mapped by this entry.
        value_type value;

        // The next field links together entries in chained hash table buckets.
        _Entry *next;

        // The firstChild and nextSiblingOrParent fields describe the tree
        // structure of paths.  An entry has one or more children when
        // firstChild is non null.  Its chlidren are stored in a singly linked
        // list, where nextSiblingOrParent points to the next entry in the list.
        //
        // The end of the list is reached when the bit stored in
        // nextSiblingOrParent is set false, indicating a pointer to the parent
        // rather than another sibling.
        _Entry *firstChild;
        TfPointerAndBits<_Entry> nextSiblingOrParent;
    };

    // Hash table's list of buckets is a vector of _Entry ptrs.
    typedef std::vector<_Entry *> _BucketVec;

public:

    // The iterator class, used to make both const and non-const 
    // iterators.  Currently only forward traversal is supported.
    template <class, class> friend class Iterator;
    template <class ValType, class EntryPtr>
    class Iterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = ValType;
        using reference = ValType&;
        using pointer = ValType*;
        using difference_type = std::ptrdiff_t;

        /// The standard requires default construction but places practically no
        /// requirements on the semantics of default-constructed iterators.
        Iterator() = default;

        /// Copy constructor (also allows for converting non-const to const).
        template <class OtherVal, class OtherEntryPtr>
        Iterator(Iterator<OtherVal, OtherEntryPtr> const &other)
            : _entry(other._entry)
            {}

        reference operator*() const { return dereference(); }
        pointer operator->() const { return &(dereference()); }

        Iterator& operator++() {
            increment();
            return *this;
        }

        Iterator operator++(int) {
            Iterator result(*this);
            increment();
            return result;
        }

        template <class OtherVal, class OtherEntryPtr>
        bool operator==(Iterator<OtherVal, OtherEntryPtr> const &other) const {
            return equal(other);
        }

        template <class OtherVal, class OtherEntryPtr>
        bool operator!=(Iterator<OtherVal, OtherEntryPtr> const &other) const {
            return !equal(other);
        }

        /// Return an iterator \a e, defining a maximal range [\a *this, \a e)
        /// such that for all \a i in the range, \a i->first is \a
        /// (*this)->first or is prefixed by \a (*this)->first.
        Iterator GetNextSubtree() const {
            Iterator result(0);
            if (_entry) {
                if (EntryPtr sibling = _entry->GetNextSibling()) {
                    // Next subtree is next sibling, if present.
                    result._entry = sibling;
                } else {
                    // Otherwise, walk up parents until we either find one with
                    // a next sibling or run out.
                    for (EntryPtr p = _entry->GetParentLink(); p;
                         p = p->GetParentLink()) {
                        if (EntryPtr sibling = p->GetNextSibling()) {
                            result._entry = sibling;
                            break;
                        }
                    }
                }
            }
            return result;
        }

        /// Returns true if incrementing this iterator would move to a child
        /// entry, false otherwise.
        bool HasChild() const {
            return bool(_entry->firstChild);
        }

    protected:
        friend class SdfPathTable;
        template <class, class> friend class Iterator;

        explicit Iterator(EntryPtr entry)
            : _entry(entry) {}

        // Fundamental functionality to implement the iterator.

        // Iterator increment.
        inline void increment() {
            // Move to first child, if present.  Otherwise move to next subtree.
            _entry = _entry->firstChild ? _entry->firstChild :
                GetNextSubtree()._entry;
        }

        // Equality comparison.  A template, to allow comparison between const
        // and non-const iterators.
        template <class OtherVal, class OtherEntryPtr>
        bool equal(Iterator<OtherVal, OtherEntryPtr> const &other) const {
            return _entry == other._entry;
        }

        // Dereference.
        ValType &dereference() const {
            return _entry->value;
        }

        // Store pointer to current entry.
        EntryPtr _entry;
    };

    typedef Iterator<value_type, _Entry *> iterator;
    typedef Iterator<const value_type, const _Entry *> const_iterator;

    /// Result type for insert().
    typedef std::pair<iterator, bool> _IterBoolPair;

    /// A handle owning a path table node that may be used to "reserve" a stable
    /// memory location for key & mapped object.  A node handle may be inserted
    /// into a table later, and if that insertion is successful, the underlying
    /// key & mapped object remain at the same memory location.
    struct NodeHandle
    {
        friend class SdfPathTable;
        
        /// Create a new NodeHandle for a table entry.  This NodeHandle can
        /// later be inserted into an SdfPathTable.  If inserted successfully,
        /// the key and value addresses remain valid.  NodeHandles may be
        /// created concurrently without additional synchronization.
        static NodeHandle
        New(value_type const &value) {
            NodeHandle ret;
            ret._unlinkedEntry.reset(new _Entry(value, nullptr));
            return ret;
        }

        /// \overload
        static NodeHandle
        New(value_type &&value) {
            NodeHandle ret;
            ret._unlinkedEntry.reset(new _Entry(std::move(value), nullptr));
            return ret;
        }

        /// \overload
        static NodeHandle
        New(key_type const &key, mapped_type const &mapped) {
            return New({ key, mapped });
        }

        /// Return a const reference to this NodeHandle's key.  This NodeHandle
        /// must be valid to call this member function (see
        /// NodeHandle::IsValid).
        key_type const &GetKey() const {
            return _unlinkedEntry->value.first;
        }

        /// Return a mutable reference to this NodeHandle's key.  This
        /// NodeHandle must be valid to call this member function (see
        /// NodeHandle::IsValid).
        key_type &GetMutableKey() {
            return _unlinkedEntry->value.first;
        }

        /// Return a const reference to this NodeHandle's mapped object.  This
        /// NodeHandle must be valid to call this member function (see
        /// NodeHandle::IsValid).
        mapped_type const &GetMapped() const {
            return _unlinkedEntry->value.second;
        }

        /// Return a mutable reference to this NodeHandle's mapped object.  This
        /// NodeHandle must be valid to call this member function (see
        /// NodeHandle::IsValid).
        mapped_type &GetMutableMapped() {
            return _unlinkedEntry->value.second;
        }

        /// Return true if this NodeHandle owns a path table entry, false
        /// otherwise.
        bool IsValid() const {
            return static_cast<bool>(_unlinkedEntry);
        }
        
        /// Return true if this NodeHandle owns a path table entry, false
        /// otherwise.
        explicit operator bool() const {
            return IsValid();
        }

        /// Delete any owned path table entry.  After calling this function,
        /// IsValid() returns false.
        void reset() {
            _unlinkedEntry.reset();
        }

    private:
        std::unique_ptr<_Entry> _unlinkedEntry;
    };
    
    /// Default constructor.
    SdfPathTable() : _size(0), _mask(0) {}

    /// Copy constructor.
    SdfPathTable(SdfPathTable const &other)
        : _buckets(other._buckets.size())
        , _size(0) // size starts at 0, since we insert elements.
        , _mask(other._mask)
    {
        // Walk all elements in the other container, inserting into this one,
        // and creating the right child/sibling links along the way.
        for (const_iterator i = other.begin(), end = other.end();
             i != end; ++i) {
            iterator j = _InsertInTable(*i).first;
            // Ensure first child and next sibling links are created.
            if (i._entry->firstChild && !j._entry->firstChild) {
                j._entry->firstChild =
                    _InsertInTable(i._entry->firstChild->value).first._entry;
            }
            // Ensure the nextSibling/parentLink is created.
            if (i._entry->nextSiblingOrParent.Get() &&  
                !j._entry->nextSiblingOrParent.Get()) {
                j._entry->nextSiblingOrParent.Set(
                    _InsertInTable(i._entry->nextSiblingOrParent.
                                   Get()->value).first._entry,
                    i._entry->nextSiblingOrParent.template BitsAs<bool>());
            }
        }
    }

    /// Move constructor.
    SdfPathTable(SdfPathTable &&other)
        : _buckets(std::move(other._buckets))
        , _size(other._size)
        , _mask(other._mask)
    {
        other._size = 0;
        other._mask = 0;
    }

    /// Destructor.
    ~SdfPathTable() {
        // Call clear to free all nodes.
        clear();
    }

    /// Copy assignment.
    SdfPathTable &operator=(SdfPathTable const &other) {
        if (this != &other)
            SdfPathTable(other).swap(*this);
        return *this;
    }

    /// Move assignment.
    SdfPathTable &operator=(SdfPathTable &&other) {
        if (this != &other)
            SdfPathTable(std::move(other)).swap(*this);
        return *this;
    }

    /// Return an iterator to the start of the table.
    iterator begin() {
        // Return an iterator pointing to the root if this table isn't empty.
        if (empty())
            return end();
        return find(SdfPath::AbsoluteRootPath());
    }

    /// Return a const_iterator to the start of the table.
    const_iterator begin() const {
        // Return an iterator pointing to the root if this table isn't empty.
        if (empty())
            return end();
        return find(SdfPath::AbsoluteRootPath());
    }

    /// Return an iterator denoting the end of the table.
    iterator end() {
        return iterator(0);
    }

    /// Return a const_iterator denoting the end of the table.
    const_iterator end() const {
        return const_iterator(0);
    }

    /// Remove the element with path \a path from the table as well as all
    /// elements whose paths are prefixed by \a path.  Return true if any
    /// elements were removed, false otherwise.
    ///
    /// Note that since descendant paths are also erased, size() may be
    /// decreased by more than one after calling this function.
    bool erase(SdfPath const &path) {
        iterator i = find(path);
        if (i == end())
            return false;
        erase(i);
        return true;
    }

    /// Remove the element pointed to by \p i from the table as well as all
    /// elements whose paths are prefixed by \a i->first.  \a i must be a valid
    /// iterator for this table.
    ///
    /// Note that since descendant paths are also erased, size() may be
    /// decreased by more than one after calling this function.
    void erase(iterator const &i) {
        // Delete descendant nodes, if any.  Then remove from parent, finally
        // erase from hash table.
        _Entry * const entry = i._entry;
        _EraseSubtree(entry);
        _RemoveFromParent(entry);
        _EraseFromTable(entry);
    }

    /// Return an iterator to the element corresponding to \a path, or \a end()
    /// if there is none.
    iterator find(SdfPath const &path) {
        if (!empty()) {
            // Find the item in the list.
            for (_Entry *e = _buckets[_Hash(path)]; e; e = e->next) {
                if (e->value.first == path)
                    return iterator(e);
            }
        }
        return end();
    }

    /// Return a const_iterator to the element corresponding to \a path, or
    /// \a end() if there is none.
    const_iterator find(SdfPath const &path) const {
        if (!empty()) {
            // Find the item in the list.
            for (_Entry const *e = _buckets[_Hash(path)]; e; e = e->next) {
                if (e->value.first == path)
                    return const_iterator(e);
            }
        }
        return end();
    }

    /// Return a pair of iterators [\a b, \a e), describing the maximal range
    /// such that for all \a i in the range, \a i->first is \a b->first or
    /// is prefixed by \a b->first.
    std::pair<iterator, iterator>
    FindSubtreeRange(SdfPath const &path) {
        std::pair<iterator, iterator> result;
        result.first = find(path);
        result.second = result.first.GetNextSubtree();
        return result;
    }

    /// Return a pair of const_iterators [\a b, \a e), describing the maximal
    /// range such that for all \a i in the range, \a i->first is \a b->first or
    /// is prefixed by \a b->first.
    std::pair<const_iterator, const_iterator>
    FindSubtreeRange(SdfPath const &path) const {
        std::pair<const_iterator, const_iterator> result;
        result.first = find(path);
        result.second = result.first.GetNextSubtree();
        return result;
    }

    /// Return 1 if there is an element for \a path in the table, otherwise 0.
    size_t count(SdfPath const &path) const {
        return find(path) != end();
    }

    /// Return the number of elements in the table.
    inline size_t size() const { return _size; }

    /// Return true if this table is empty.
    inline bool empty() const { return !size(); }

    /// Insert \a value into the table, and additionally insert default entries
    /// for all ancestral paths of \a value.first that do not already exist in
    /// the table.
    ///
    /// Return a pair of iterator and bool.  The iterator points to the inserted
    /// element, the bool indicates whether insertion was successful.  The bool
    /// is true if \a value was successfully inserted and false if an element
    /// with path \a value.first was already present in the map.
    ///
    /// Note that since ancestral paths are also inserted, size() may be
    /// increased by more than one after calling this function.
    _IterBoolPair insert(value_type const &value) {
        // Insert in table.
        _IterBoolPair result = _InsertInTable(value);
        if (result.second) {
            // New element -- make sure the parent is inserted.
            _UpdateTreeForNewEntry(result);
        }
        return result;
    }

    /// \overload
    ///
    /// Insert the entry held by \p node into this table.  If the insertion is
    /// successful, the contents of \p node are moved-from and indeterminate.
    /// Otherwise if the insertion is unsuccessful, the contents of \p node are
    /// unmodified.
    _IterBoolPair insert(NodeHandle &&node) {
        // Insert in table.
        _IterBoolPair result = _InsertInTable(std::move(node));
        if (result.second) {
            // New element -- make sure the parent is inserted.
            _UpdateTreeForNewEntry(result);
        }
        return result;
    }

    /// Shorthand for the following, where \a t is an SdfPathTable<T>.
    /// \code
    ///     t.insert(value_type(path, mapped_type())).first->second
    /// \endcode
    mapped_type &operator[](SdfPath const &path) {
        return insert(value_type(path, mapped_type())).first->second;
    }

    /// Remove all elements from the table, leaving size() == 0.  Note that this
    /// function will not shrink the number of buckets used for the hash table.
    /// To do that, swap this instance with a default constructed instance.
    /// See also \a TfReset.
    void clear() {
        // Note this leaves the size of _buckets unchanged.
        for (size_t i = 0, n = _buckets.size(); i != n; ++i) {
            _Entry *entry = _buckets[i];
            while (entry) {
                _Entry *next = entry->next;
                delete entry;
                entry = next;
            }
            _buckets[i] = 0;
        }
        _size = 0;
    }

    /// Equivalent to clear(), but destroy contained objects in parallel.  This
    /// requires that running the contained objects' destructors is thread-safe.
    void ClearInParallel() {
        auto visitFn =
            [](void*& voidEntry) {
                _Entry *entry = static_cast<_Entry *>(voidEntry);
                while (entry) {
                    _Entry *next = entry->next;
                    delete entry;
                    entry = next;
                }
                voidEntry = nullptr;
            };
        Sdf_VisitPathTableInParallel(reinterpret_cast<void **>(_buckets.data()),
                                     _buckets.size(), visitFn);
        _size = 0;
    }        

    /// Swap this table's contents with \a other.
    void swap(SdfPathTable &other) {
        _buckets.swap(other._buckets);
        std::swap(_size, other._size);
        std::swap(_mask, other._mask);
    }

    /// Return a vector of the count of elements in each bucket.
    std::vector<size_t> GetBucketSizes() const {
        std::vector<size_t> sizes(_buckets.size(), 0u);;
        for (size_t i = 0, n = _buckets.size(); i != n; ++i) {
            for (_Entry *entry = _buckets[i]; entry; entry = entry->next) {
                sizes[i]++;
            }
        }
        return sizes;
    }

    /// Replaces all prefixes from \p oldName to \p newName.
    /// Note that \p oldName and \p newName need to be silbing paths (ie. 
    /// their parent paths must be the same).
    void UpdateForRename(const SdfPath &oldName, const SdfPath &newName) {

        if (oldName.GetParentPath() != newName.GetParentPath()) {
            TF_CODING_ERROR("Unexpected arguments.");
            return;
        }
    
        std::pair<iterator, iterator> range = FindSubtreeRange(oldName);
        for (iterator i=range.first; i!=range.second; ++i) {
            insert(value_type(
                i->first.ReplacePrefix(oldName, newName),
                i->second));
        }
        
        if (range.first != range.second)
            erase(oldName);
    }

    /// ParallelForEach: parallel iteration over all of the key-value pairs
    /// in the path table.  The type of \p visitFn should be a callable,
    /// taking a (const SdfPath&, mapped_type&), representing the loop 
    /// body.  Note: since this function is run in parallel, visitFn is
    /// responsible for synchronizing access to any non-pathtable state.
    template <typename Callback>
    void ParallelForEach(Callback const& visitFn) {
        auto lambda =
            [&visitFn](void*& voidEntry) {
                _Entry *entry = static_cast<_Entry *>(voidEntry);
                while (entry) {
                    visitFn(std::cref(entry->value.first),
                            std::ref(entry->value.second));
                    entry = entry->next;
                }
            };
        Sdf_VisitPathTableInParallel(reinterpret_cast<void **>(_buckets.data()),
                                     _buckets.size(), lambda);
    }

    /// ParallelForEach: const version, runnable on a const path table and
    /// taking a (const SdfPath&, const mapped_type&) input.
    template <typename Callback>
    void ParallelForEach(Callback const& visitFn) const {
        auto lambda =
            [&visitFn](void*& voidEntry) {
                const _Entry *entry = static_cast<const _Entry *>(voidEntry);
                while (entry) {
                    visitFn(std::cref(entry->value.first),
                            std::cref(entry->value.second));
                    entry = entry->next;
                }
            };
        Sdf_VisitPathTableInParallel(
            // Note: the const cast here is undone by the static cast above...
            reinterpret_cast<void **>(const_cast<_Entry**>(_buckets.data())),
            _buckets.size(), lambda);
    }

private:

    // Helper to get parent paths.
    static SdfPath _GetParentPath(SdfPath const &path) {
        return path.GetParentPath();
    }

    void _UpdateTreeForNewEntry(_IterBoolPair const &iresult) {
        // New element -- make sure the parent is inserted.
        _Entry * const newEntry = iresult.first._entry;
        SdfPath const &parentPath = _GetParentPath(newEntry->value.first);
        if (!parentPath.IsEmpty()) {
            iterator parIter =
                insert(value_type(parentPath, mapped_type())).first;
            // Add the new entry to the parent's children.
            parIter._entry->AddChild(newEntry);
        }
    }

    // Helper to insert \a value in the hash table.  Is responsible for growing
    // storage space when necessary.  Does not consider the tree structure.
    template <class MakeEntryFn>
    _IterBoolPair _InsertInTableImpl(key_type const &key,
                                     MakeEntryFn &&makeEntry) {
        // If we have no storage at all so far, grow.
        if (_mask == 0)
            _Grow();

        // Find the item, if present.
        _Entry **bucketHead = &(_buckets[_Hash(key)]);
        for (_Entry *e = *bucketHead; e; e = e->next) {
            if (e->value.first == key) {
                return _IterBoolPair(iterator(e), false);
            }
        }

        // Not present.  If the table is getting full then grow and re-find the
        // bucket.
        if (_IsTooFull()) {
            _Grow();
            bucketHead = &(_buckets[_Hash(key)]);
        }

        // Make an entry and insert it in the list.
        *bucketHead = std::forward<MakeEntryFn>(makeEntry)(*bucketHead);

        // One more element
        ++_size;

        // Return the new item
        return _IterBoolPair(iterator(*bucketHead), true);
    }        
    
    _IterBoolPair _InsertInTable(value_type const &value) {
        return _InsertInTableImpl(
            value.first, [&value](_Entry *next) {
                return new _Entry(value, next);
            });
    }                       

    _IterBoolPair _InsertInTable(NodeHandle &&node) {
        return _InsertInTableImpl(
            node.GetKey(), [&node](_Entry *next) mutable {
                node._unlinkedEntry->next = next;
                return node._unlinkedEntry.release();
            });
    }
                                  
    // Erase \a entry from the hash table.  Does not consider tree structure.
    void _EraseFromTable(_Entry *entry) {
        // Remove from table.
        _Entry **cur = &_buckets[_Hash(entry->value.first)];
        while (*cur != entry)
            cur = &((*cur)->next);

        // Unlink item and destroy it
        --_size;
        _Entry *tmp = *cur;
        *cur = tmp->next;
        delete tmp;
    }

    // Erase all the tree structure descendants of \a entry from the table.
    void _EraseSubtree(_Entry *entry) {
        // Delete descendant nodes, if any.
        if (_Entry * const firstChild = entry->firstChild) {
            _EraseSubtreeAndSiblings(firstChild);
            _EraseFromTable(firstChild);
        }
    }

    // Erase all the tree structure descendants and siblings of \a entry from
    // the table.
    void _EraseSubtreeAndSiblings(_Entry *entry) {
        // Remove subtree.
        _EraseSubtree(entry);

        // And siblings.
        _Entry *sibling = entry->GetNextSibling();
        _Entry *nextSibling = sibling ? sibling->GetNextSibling() : nullptr;
        while (sibling) {
            _EraseSubtree(sibling);
            _EraseFromTable(sibling);
            sibling = nextSibling;
            nextSibling = sibling ? sibling->GetNextSibling() : nullptr;
        }
    }

    // Remove \a entry from its parent's list of children in the tree structure
    // alone.  Does not consider the table.
    void _RemoveFromParent(_Entry *entry) {
        if (entry->value.first == SdfPath::AbsoluteRootPath())
            return;

        // Find parent in table.
        iterator parIter = find(_GetParentPath(entry->value.first));

        // Remove this entry from the parent's children.
        parIter._entry->RemoveChild(entry);
    }

    // Grow the table's number of buckets to the next larger size.  Rehashes the
    // elements into the new table, but leaves tree structure untouched.  (The
    // tree structure need not be modified).
    void _Grow() {
        TfAutoMallocTag2 tag2("Sdf", "SdfPathTable::_Grow");
        TfAutoMallocTag tag(__ARCH_PRETTY_FUNCTION__);

        // Allocate a new bucket list of twice the size.  Minimum nonzero number
        // of buckets is 8.
        _mask = std::max(size_t(7), (_mask << 1) + 1);
        _BucketVec newBuckets(_mask + 1);

        // Move items to a new bucket list
        for (size_t i = 0, n = _buckets.size(); i != n; ++i) {
            _Entry *elem = _buckets[i];
            while (elem) {
                // Save pointer to next item
                _Entry *next = elem->next;

                // Get the bucket in the new list
                _Entry *&m = newBuckets[_Hash(elem->value.first)];

                // Insert the item at the head
                elem->next = m;
                m = elem;

                // Next item
                elem = next;
            }
        }

        // Use the new buckets
        _buckets.swap(newBuckets);
    }

    // Return true if the table should be made bigger.
    bool _IsTooFull() const {
        return _size > _buckets.size();
    }

    // Return the bucket index for \a path.
    inline size_t _Hash(SdfPath const &path) const {
        return path.GetHash() & _mask;
    }

private:
    _BucketVec _buckets;
    size_t _size;
    size_t _mask;

};

}  // namespace pxr

#endif // PXR_SDF_PATH_TABLE_H
