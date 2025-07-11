// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_CHILDREN_VIEW_H
#define PXR_SDF_CHILDREN_VIEW_H

/// \file sdf/childrenView.h

#include "./api.h"
#include "./children.h"
#include <pxr/tf/iterator.h>

#include <algorithm>
#include <vector>

namespace pxr {

/// \class SdfChildrenViewTrivialPredicate
///
/// Special case predicate that always passes.
///
/// \c T is the type exposed by the value traits.
///
/// This predicate is compiled out.
///
template <class T>
class SdfChildrenViewTrivialPredicate {
public:
    bool operator()(const T& x) const { return true; }
};

/// \class SdfChildrenViewTrivialAdapter
///
/// Special case adapter that does no conversions.
///
template <class T>
class SdfChildrenViewTrivialAdapter {
public:
    typedef T PrivateType;
    typedef T PublicType;
    static const PublicType& Convert(const PrivateType& t) { return t; }
};

/// \class Sdf_ChildrenViewTraits
/// This traits class defines the iterator for a particular ChildrenView
/// along with conversions to and from the view's internal un-filtered iterator.
///
/// A specialization of the traits for trivial predicates allows the
/// internal iterator to be used directly.
///
template <typename _Owner, typename _InnerIterator, typename _DummyPredicate>
class Sdf_ChildrenViewTraits {
private:

    // Owner's predicate object will be used by the filter iterator.
    // In C++20, consider using the ranges library to simplify this
    class _FilterIterator {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = typename _InnerIterator::value_type;
        using reference = typename _InnerIterator::reference;
        using pointer = typename _InnerIterator::pointer;
        using difference_type = typename _InnerIterator::difference_type;

        _FilterIterator() = default;
        _FilterIterator(const _Owner* owner,
                        const _InnerIterator& underlyingIterator,
                        const _InnerIterator& end) :
                            _owner(owner),
                            _underlyingIterator(underlyingIterator),
                            _end(end) {
            _Filter();
        }

        reference operator*() const {
            return *_underlyingIterator;
        }

        pointer operator->() const {
            return _underlyingIterator.operator->();
        }

        _FilterIterator& operator++() {
            TF_DEV_AXIOM(_underlyingIterator != _end);
            ++_underlyingIterator;
            _Filter();
            return *this;
        }

        _FilterIterator operator++(int) {
            TF_DEV_AXIOM(_underlyingIterator != _end);
            _FilterIterator result(*this);
            ++_underlyingIterator;
            _Filter();
            return result;
        }

        bool operator==(const _FilterIterator& other) const {
            return _underlyingIterator == other._underlyingIterator;
        }

        bool operator!=(const _FilterIterator& other) const {
            return _underlyingIterator != other._underlyingIterator;
        }

        const _InnerIterator& GetBase() const { return _underlyingIterator; }

    private:
        // Skip any iterators that don't satisfy the predicate
        bool _ShouldFilter(const value_type& x) const
        {
            return !_owner->GetPredicate()(
                _Owner::Adapter::Convert(x));
        }

        void _Filter()
        {
            while (_underlyingIterator != _end &&
                   _ShouldFilter(*_underlyingIterator)) {
                ++_underlyingIterator;
            }
        }

        const _Owner* _owner = nullptr;
        _InnerIterator _underlyingIterator;
        _InnerIterator _end;
    };

public:
    using const_iterator = _FilterIterator;

    // Convert from a private _InnerIterator to a public const_iterator.
    // filter_iterator requires an end iterator, which is constructed using
    // size.
    static const_iterator GetIterator(const _Owner* owner,
                                      const _InnerIterator& i,
                                      size_t size)
    {
        _InnerIterator end(owner,size);
        return const_iterator(owner, i, end);
    }

    // Convert from a public const_iterator to a private _InnerIterator.
    static const _InnerIterator& GetBase(const const_iterator& i)
    {
        return i.GetBase();
    }
};

// Children view traits specialization for trivial predicates.  This
// eliminates the predicate altogether and defines the public iterator type
// to be the same as the inner iterator type.
template <typename _Owner, typename _InnerIterator>
class Sdf_ChildrenViewTraits<_Owner, _InnerIterator,
    SdfChildrenViewTrivialPredicate<typename _Owner::ChildPolicy::ValueType> > {
private:

public:
    typedef _InnerIterator const_iterator;

    static const const_iterator& GetIterator(const _Owner*,
        const _InnerIterator& i, size_t size)
    {
        return i;
    }

    static const _InnerIterator& GetBase(const const_iterator& i)
    {
        return i;
    }
};

/// \class SdfChildrenView
///
/// Provides a view onto an object's children.
///
/// The \c _ChildPolicy dictates the type of children being viewed by this
/// object. This policy defines the key type by which children are referenced
/// (e.g. a TfToken, or an SdfPath) and the value type of the children objects.
///
/// The \c _Predicate takes a value type argument and returns \c true if the
/// object should be included in the view and \c false otherwise.
///
/// The \c _Adapter allows the view to present the children objects as a
/// different type. The _Adapter class must provide functions to convert the
/// children object type defined by \c _ChildPolicy to the desired public
/// type and vice-versa. See SdfChildrenViewTrivialAdapter for an example.
/// By default, the view presents children objects as the value type defined
/// in \c _ChildPolicy.
///
/// Note that all methods are const, i.e. the children cannot be changed
/// through a view.
///
template <typename _ChildPolicy,
          typename _Predicate =
              SdfChildrenViewTrivialPredicate<
                  typename _ChildPolicy::ValueType>,
          typename _Adapter =
              SdfChildrenViewTrivialAdapter<
                  typename _ChildPolicy::ValueType> >
class SdfChildrenView {
public:
    typedef SdfChildrenView<_ChildPolicy, _Predicate, _Adapter> This;

    typedef _Adapter                        Adapter;
    typedef _Predicate                      Predicate;
    typedef _ChildPolicy                    ChildPolicy;
    typedef typename ChildPolicy::KeyPolicy KeyPolicy;
    typedef Sdf_Children<ChildPolicy>        ChildrenType;

    typedef typename ChildPolicy::KeyType   key_type;
    typedef typename Adapter::PublicType    value_type;

private:

    // An iterator type for the internal unfiltered data storage. This
    // iterator holds a pointer to its owning object and an index into
    // the owner's storage. That allows the iterator to operate without
    // knowing anything about the specific data storage that's used,
    // which is important for providing both Gd and Lsd backed storage.
    class _InnerIterator {
        class _PtrProxy {
        public:
            SdfChildrenView::value_type* operator->() { return &_value; }
        private:
            friend class SdfChildrenView;
            explicit _PtrProxy(const SdfChildrenView::value_type& value)
                : _value(value) {}
            SdfChildrenView::value_type _value;
        };
    public:
        using iterator_category = std::random_access_iterator_tag;
        using value_type = SdfChildrenView::value_type;
        using reference = value_type;
        using pointer = _PtrProxy;
        using difference_type = std::ptrdiff_t;

        _InnerIterator() = default;
        _InnerIterator(const This* owner, const size_t& pos) :
            _owner(owner), _pos(pos) { }

        reference operator*() const { return dereference(); }
        pointer operator->() const { return pointer(dereference()); }
        reference operator[](const difference_type index) const {
            _InnerIterator advanced(*this);
            advanced.advance(index);
            return advanced.dereference();
        }

        difference_type operator-(const _InnerIterator& other) const {
            return -distance_to(other);
        }

        _InnerIterator& operator++() {
            increment();
            return *this;
        }

        _InnerIterator& operator--() {
            decrement();
            return *this;
        }

        _InnerIterator operator++(int) {
            _InnerIterator result(*this);
            increment();
            return result;
        }

        _InnerIterator operator--(int) {
            _InnerIterator result(*this);
            decrement();
            return result;
        }

        _InnerIterator operator+(const difference_type increment) const {
            _InnerIterator result(*this);
            result.advance(increment);
            return result;
        }

        _InnerIterator operator-(const difference_type decrement) const {
            _InnerIterator result(*this);
            result.advance(-decrement);
            return result;
        }

        _InnerIterator& operator+=(const difference_type increment) {
            advance(increment);
            return *this;
        }

        _InnerIterator& operator-=(const difference_type decrement) {
            advance(-decrement);
            return *this;
        }

        bool operator==(const _InnerIterator& other) const {
            return equal(other);
        }

        bool operator!=(const _InnerIterator& other) const {
            return !equal(other);
        }

        bool operator<(const _InnerIterator& other) const {
            TF_DEV_AXIOM(_owner == other._owner);
            return _pos < other._pos;
        }

        bool operator<=(const _InnerIterator& other) const {
            TF_DEV_AXIOM(_owner == other._owner);
            return _pos <= other._pos;
        }

        bool operator>(const _InnerIterator& other) const {
            TF_DEV_AXIOM(_owner == other._owner);
            return _pos > other._pos;
        }

        bool operator>=(const _InnerIterator& other) const {
            TF_DEV_AXIOM(_owner == other._owner);
            return _pos >= other._pos;
        }

    private:

        reference dereference() const
        {
            return _owner->_Get(_pos);
        }

        bool equal(const _InnerIterator& other) const
        {
            return _pos == other._pos;
        }

        void increment() {
            ++_pos;
        }

        void decrement() {
            --_pos;
        }

        void advance(difference_type n) {
            _pos += n;
        }

        difference_type distance_to(const _InnerIterator& other) const {
            return other._pos-_pos;
        }

    private:
        const This* _owner = nullptr;
        size_t _pos = 0;
    };

public:
    typedef Sdf_ChildrenViewTraits<This, _InnerIterator, Predicate> _Traits;
    typedef typename _Traits::const_iterator const_iterator;
    typedef Tf_ProxyReferenceReverseIterator<const_iterator> const_reverse_iterator;
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;

    SdfChildrenView()
    {
    }
        
    SdfChildrenView(const SdfLayerHandle &layer, const SdfPath &path,
                    const TfToken &childrenKey,
                    const KeyPolicy& keyPolicy = KeyPolicy()) :
        _children(layer, path, childrenKey, keyPolicy)
    {
    }

    SdfChildrenView(const SdfLayerHandle &layer, const SdfPath &path,
                    const TfToken &childrenKey,
                    const Predicate& predicate,
                    const KeyPolicy& keyPolicy = KeyPolicy()) :
        _children(layer, path, childrenKey, keyPolicy),
        _predicate(predicate)
    {
    }

    SdfChildrenView(const SdfChildrenView &other) :
        _children(other._children),
        _predicate(other._predicate)
    {
    }

    template <class OtherAdapter>
    SdfChildrenView(const SdfChildrenView<ChildPolicy, Predicate, 
                                        OtherAdapter> &other) :
        _children(other._children),
        _predicate(other._predicate)
    {
    }

    ~SdfChildrenView()
    {
    }

    SdfChildrenView& operator=(const SdfChildrenView &other)
    {
        _children= other._children;
        _predicate = other._predicate;
        return *this;
    }

    /// Returns an const_iterator pointing to the beginning of the vector.
    const_iterator begin() const {
        _InnerIterator i(this,0);
        return _Traits::GetIterator(this, i, _GetSize());
    }

    /// Returns an const_iterator pointing to the end of the vector.
    const_iterator end() const {
        _InnerIterator i(this,_GetSize());
        return _Traits::GetIterator(this, i, _GetSize());
    }

    /// Returns an const_reverse_iterator pointing to the beginning of the
    /// reversed vector.
    const_reverse_iterator rbegin() const {
        return const_reverse_iterator(end());
    }

    /// Returns an const_reverse_iterator pointing to the end of the
    /// reversed vector.
    const_reverse_iterator rend() const {
        return const_reverse_iterator(begin());
    }

    /// Returns the size of the vector.
    size_type size() const {
        return std::distance(begin(), end());
    }

    /// Returns \c true if the vector is empty.
    bool empty() const {
        return begin() == end();
    }

    /// Returns the \p n'th element.
    value_type operator[](size_type n) const {
        const_iterator i = begin();
        std::advance(i, n);
        return *i;
    }

    /// Returns the first element.
    value_type front() const {
        return *begin();
    }

    /// Returns the last element.
    value_type back() const {
        return *rbegin();
    }

    /// Finds the element with key \p x.
    const_iterator find(const key_type& x) const {
        _InnerIterator inner(this, _children.Find(x));
        const_iterator iter = _Traits::GetIterator(this, inner, _GetSize());

        // _Traits::GetIterator may return a filtered iterator. We need to
        // check that that iterator actually corresponds to the desired item.
        // This ensures that we return end() in the case where the element being
        // searched for is present in the children but filtered out by the 
        // view's predicate.
        return _Traits::GetBase(iter) == inner ? iter : end();
    }

    /// Finds element \p x, if present in this view.
    const_iterator find(const value_type& x) const {
        const_iterator i = find(key(x));
        return (i != end() && *i == x) ? i : end();
    }

    /// Returns the key for an element.
    key_type key(const const_iterator& x) const {
        return key(*x);
    }

    /// Returns the key for a value.
    key_type key(const value_type& x) const {
        return _children.FindKey(Adapter::Convert(x));
    }

    /// Returns the elements, in order.
    std::vector<value_type> values() const {
        return std::vector<value_type>(begin(), end());
    }

    /// Returns the elements, in order.
    template <typename V>
    V values_as() const {
        V x;
        std::copy(begin(), end(), std::inserter(x, x.begin()));
        return x;
    }

    /// Returns the keys for all elements, in order.
    std::vector<key_type> keys() const {
        std::vector<key_type> result;
        result.reserve(size());
        for (const_iterator i = begin(), n = end(); i != n; ++i) {
            result.push_back(key(i));
        }
        return result;
    }

    /// Returns the keys for all elements, in order.
    template <typename V>
    V keys_as() const {
        std::vector<key_type> k = keys();
        return V(k.begin(), k.end());
    }

    /// Returns the elements as a dictionary.
    template <typename Dict>
    Dict items_as() const {
        Dict result;
        for (const_iterator i = begin(), n = end(); i != n; ++i) {
            result.insert(std::make_pair(key(i), *i));
        }
        return result;
    }

    /// Returns true if an element with key \p x is in the container.
    bool has(const key_type& x) const {
        return (_children.Find(x) != _GetSize());
    }

    /// Returns true if an element with the same key as \p x is in
    /// the container.
    bool has(const value_type& x) const {
        return has(key(Adapter::Convert(x)));
    }

    /// Returns the number of elements with key \p x in the container.
    size_type count(const key_type& x) const {
        return has(x);
    }

    /// Returns the element with key \p x or a default constructed value
    /// if no such element exists.
    value_type get(const key_type& x) const {
        size_t index = _children.Find(x);
        if (index == _GetSize()) {
            return value_type();
        }
        return _Get(index);
    }

    /// Returns the element with key \p x or the fallback if no such
    /// element exists.
    value_type get(const key_type& x, const value_type& fallback) const {
        size_t index = _children.Find(x);
        if (index == _GetSize()) {
            return fallback;
        }
        return _Get(index);
    }

    /// Returns the element with key \p x or a default constructed value
    /// if no such element exists.
    value_type operator[](const key_type& x) const {
        return get(x);
    }

    /// Compares children for equality.  Children are equal if the
    /// list edits are identical and the keys contain the same elements.
    bool operator==(const This& other) const {
        return _children.IsEqualTo(other._children);
    }

    /// Compares children for inequality.  Children are not equal if
    /// list edits are not identical or the keys don't contain the same
    /// elements.
    bool operator!=(const This& other) const {
        return !_children.IsEqualTo(other._children);
    }

    // Return true if this object is valid
    bool IsValid() const {
        return _children.IsValid();
    }

    // Return the Sd_Children object that this view is holding.
    ChildrenType &GetChildren() {
        return _children;
    }

    // Return this view's predicate.
    const Predicate& GetPredicate() const {
        return _predicate;
    }

private:
    // Return the value that corresponds to the provided index.
    value_type _Get(size_type index) const {
        return Adapter::Convert(_children.GetChild(index));
    }

    // Return the number of elements
    size_t _GetSize() const {
        return _children.GetSize();
    }

private:
    template <class V, class P, class A> friend class SdfChildrenView;
    ChildrenType _children;
    Predicate _predicate;
};

/// Helper class to convert a given view of type \c _View to an 
/// adapted view using \c _Adapter as the adapter class.
template <class _View, class _Adapter>
struct SdfAdaptedChildrenViewCreator
{
    typedef _View OriginalView;
    typedef SdfChildrenView<typename _View::ChildPolicy,
                            typename _View::Predicate,
                            _Adapter> AdaptedView;

    static AdaptedView Create(const OriginalView& view)
    {
        return AdaptedView(view);
    }
};

// Allow TfIteration over children views.
template <typename C, typename P, typename A>
struct Tf_ShouldIterateOverCopy<SdfChildrenView<C, P, A> > : std::true_type
{
};
template <typename C, typename P, typename A>
struct Tf_IteratorInterface<SdfChildrenView<C, P, A>, false> {
    typedef SdfChildrenView<C, P, A> Type;
    typedef typename Type::const_iterator IteratorType;
    static IteratorType Begin(Type const &c) { return c.begin(); }
    static IteratorType End(Type const &c) { return c.end(); }
};
template <typename C, typename P, typename A>
struct Tf_IteratorInterface<SdfChildrenView<C, P, A>, true> {
    typedef SdfChildrenView<C, P, A> Type;
    typedef typename Type::const_reverse_iterator IteratorType;
    static IteratorType Begin(Type const &c) { return c.rbegin(); }
    static IteratorType End(Type const &c) { return c.rend(); }
};

}  // namespace pxr

#endif // PXR_SDF_CHILDREN_VIEW_H
