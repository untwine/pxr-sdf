// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./listOp.h"
#include "./path.h"
#include "./payload.h"
#include "./reference.h"
#include "./types.h"
#include <pxr/tf/denseHashSet.h>
#include <pxr/tf/diagnostic.h>
#include <pxr/tf/iterator.h>
#include <pxr/tf/registryManager.h>
#include <pxr/tf/type.h>
#include <pxr/tf/token.h>
#include <pxr/trace/trace.h>
#include <pxr/tf/pxrTslRobinMap/robin_set.h>

#include <ostream>

using std::string;
using std::vector;

namespace pxr {

TF_REGISTRY_FUNCTION(pxr::TfType)
{
    TfType::Define<SdfTokenListOp>()
        .Alias(TfType::GetRoot(), "SdfTokenListOp");
    TfType::Define<SdfPathListOp>()
        .Alias(TfType::GetRoot(), "SdfPathListOp");
    TfType::Define<SdfStringListOp>()
        .Alias(TfType::GetRoot(), "SdfStringListOp");
    TfType::Define<SdfReferenceListOp>()
        .Alias(TfType::GetRoot(), "SdfReferenceListOp");
    TfType::Define<SdfPayloadListOp>()
        .Alias(TfType::GetRoot(), "SdfPayloadListOp");
    TfType::Define<SdfIntListOp>()
        .Alias(TfType::GetRoot(), "SdfIntListOp");
    TfType::Define<SdfUIntListOp>()
        .Alias(TfType::GetRoot(), "SdfUIntListOp");
    TfType::Define<SdfInt64ListOp>()
        .Alias(TfType::GetRoot(), "SdfInt64ListOp");
    TfType::Define<SdfUInt64ListOp>()
        .Alias(TfType::GetRoot(), "SdfUInt64ListOp");
    TfType::Define<SdfUnregisteredValueListOp>()
        .Alias(TfType::GetRoot(), "SdfUnregisteredValueListOp");

    TfType::Define<SdfListOpType>();
}

TF_REGISTRY_FUNCTION(pxr::TfEnum)
{
    TF_ADD_ENUM_NAME(SdfListOpTypeExplicit);
    TF_ADD_ENUM_NAME(SdfListOpTypeAdded);
    TF_ADD_ENUM_NAME(SdfListOpTypePrepended);
    TF_ADD_ENUM_NAME(SdfListOpTypeAppended);
    TF_ADD_ENUM_NAME(SdfListOpTypeDeleted);
    TF_ADD_ENUM_NAME(SdfListOpTypeOrdered);
}


template <typename T>
SdfListOp<T>::SdfListOp()
    : _isExplicit(false)
{
}

template <typename T>
SdfListOp<T>
SdfListOp<T>::CreateExplicit(
    const ItemVector& explicitItems)
{
    SdfListOp<T> listOp;
    listOp.SetExplicitItems(explicitItems);
    return listOp;
}

template <typename T>
SdfListOp<T>
SdfListOp<T>::Create(
    const ItemVector& prependedItems,
    const ItemVector& appendedItems,
    const ItemVector& deletedItems)
{
    SdfListOp<T> listOp;
    listOp.SetPrependedItems(prependedItems);
    listOp.SetAppendedItems(appendedItems);
    listOp.SetDeletedItems(deletedItems);
    return listOp;
}

template <typename T>
void 
SdfListOp<T>::Swap(SdfListOp<T>& rhs)
{
    std::swap(_isExplicit, rhs._isExplicit);
    _explicitItems.swap(rhs._explicitItems);
    _addedItems.swap(rhs._addedItems);
    _prependedItems.swap(rhs._prependedItems);
    _appendedItems.swap(rhs._appendedItems);
    _deletedItems.swap(rhs._deletedItems);
    _orderedItems.swap(rhs._orderedItems);
}

template <typename T>
bool
SdfListOp<T>::HasItem(const T& item) const
{
    if (IsExplicit()) {
        return std::find(_explicitItems.begin(), _explicitItems.end(), item)
            != _explicitItems.end();
    }

    return 
        (std::find(_addedItems.begin(), _addedItems.end(), item)
            != _addedItems.end()) ||
        (std::find(_prependedItems.begin(), _prependedItems.end(), item)
            != _prependedItems.end()) ||
        (std::find(_appendedItems.begin(), _appendedItems.end(), item)
            != _appendedItems.end()) ||
        (std::find(_deletedItems.begin(), _deletedItems.end(), item)
            != _deletedItems.end()) ||
        (std::find(_orderedItems.begin(), _orderedItems.end(), item)
            != _orderedItems.end());
}

template <typename T>
const typename SdfListOp<T>::ItemVector &
SdfListOp<T>::GetItems(SdfListOpType type) const
{
    switch(type) {
    case SdfListOpTypeExplicit:
        return _explicitItems;
    case SdfListOpTypeAdded:
        return _addedItems;
    case SdfListOpTypePrepended:
        return _prependedItems;
    case SdfListOpTypeAppended:
        return _appendedItems;
    case SdfListOpTypeDeleted:
        return _deletedItems;
    case SdfListOpTypeOrdered:
        return _orderedItems;
    }

    TF_CODING_ERROR("Got out-of-range type value: %d", type);
    return _explicitItems;
}

template <typename T>
typename SdfListOp<T>::ItemVector 
SdfListOp<T>::GetAppliedItems() const
{
    ItemVector result;
    ApplyOperations(&result);
    return result;
}

template <typename T>
bool
SdfListOp<T>::_MakeUnique(std::vector<T>& items, bool reverse, std::string* errMsg)
{
    // Many of the vectors we see here are either just a few elements long
    // (references, payloads) or are already sorted and unique (topology
    // indexes, etc).
    if (items.size() <= 1) {
        return true;
    }

    // Many are of small size, just check all pairs.
    if (items.size() <= 10) {
        bool isUnique = true;
        using iter = typename std::vector<T>::const_iterator;
        iter iend = std::prev(items.end()), jend = items.end();
        for (iter i = items.begin(); i != iend; ++i) {
            for (iter j = std::next(i); j != jend; ++j) {
                if (*i == *j) {
                    isUnique = false;
                }
            }
        }
        if (isUnique) {
            return true;
        }
    }

    // Check for strictly sorted order.
    auto comp = _ItemComparator();
    if (std::adjacent_find(items.begin(), items.end(),
                        [comp](T const &l, T const &r) {
                            return !comp(l, r);
                        }) == items.end()) {
        return true;
    } 

    // Otherwise do a more expensive copy & sort to check for dupes.
    std::vector<T> copy(items);
    std::sort(copy.begin(), copy.end(), _ItemComparator());
    auto duplicate = std::adjacent_find(copy.begin(), copy.end());
    
    if (duplicate == copy.end()) 
    {
        return true; 
    }

    // If duplicates are present, remove them
    pxr::tsl::robin_set<T, TfHash> existingSet;
    std::vector<T> uniqueItems;
    uniqueItems.reserve(items.size());

    if (reverse) {
        for (auto it = items.rbegin(); it != items.rend(); it++) {
            if (existingSet.insert(*it).second) {
                uniqueItems.push_back(*it);
            }
        }
        std::reverse(uniqueItems.begin(), uniqueItems.end());
    } else {
        for (auto it = items.cbegin(); it != items.cend(); it++) {
            if (existingSet.insert(*it).second) {
                uniqueItems.push_back(*it);
            }
        }
    }
        
    items = uniqueItems;
    if (errMsg) {
        *errMsg = "Duplicate item found in SdfListOp: %s.", TfStringify(*duplicate);
    }
    return false;
}

template <typename T>
bool 
SdfListOp<T>::SetExplicitItems(const ItemVector &items, std::string *errMsg)
{
    _SetExplicit(true);
    _explicitItems = items;
    return _MakeUnique(_explicitItems, false, errMsg);
}

template <typename T>
void 
SdfListOp<T>::SetAddedItems(const ItemVector &items)
{
    _SetExplicit(false);
    _addedItems = items;
}

template <typename T>
bool 
SdfListOp<T>::SetPrependedItems(const ItemVector &items, std::string *errMsg)
{
    _SetExplicit(false);
    _prependedItems = items;
    return _MakeUnique(_prependedItems, false, errMsg);
}

template <typename T>
bool 
SdfListOp<T>::SetAppendedItems(const ItemVector &items, std::string *errMsg)
{
    _SetExplicit(false);
    _appendedItems = items;
    return _MakeUnique(_appendedItems, true, errMsg);
}

template <typename T>
bool 
SdfListOp<T>::SetDeletedItems(const ItemVector &items, std::string *errMsg)
{
    _SetExplicit(false);
    _deletedItems = items;
    return _MakeUnique(_deletedItems, false, errMsg);
}

template <typename T>
void 
SdfListOp<T>::SetOrderedItems(const ItemVector &items)
{
    _SetExplicit(false);
    _orderedItems = items;
}

template <typename T>
bool
SdfListOp<T>::SetItems(const ItemVector &items, SdfListOpType type, std::string *errMsg)
{
    switch(type) {
    case SdfListOpTypeExplicit:
        return SetExplicitItems(items, errMsg);
    case SdfListOpTypeAdded:
        SetAddedItems(items);
        break;
    case SdfListOpTypePrepended:
        return SetPrependedItems(items, errMsg);
    case SdfListOpTypeAppended:
        return SetAppendedItems(items, errMsg);
    case SdfListOpTypeDeleted:
        return SetDeletedItems(items, errMsg);
    case SdfListOpTypeOrdered:
        SetOrderedItems(items);
        break;
    }
    return true;
}

template <typename T>
void
SdfListOp<T>::_SetExplicit(bool isExplicit)
{
    if (isExplicit != _isExplicit) {
        _isExplicit = isExplicit;
        _explicitItems.clear();
        _addedItems.clear();
        _prependedItems.clear();
        _appendedItems.clear();
        _deletedItems.clear();
        _orderedItems.clear();
    }
}

template <typename T>
void
SdfListOp<T>::Clear()
{
    // _SetExplicit will clear all items and set the explicit flag as specified.
    // Temporarily change explicit flag to bypass check in _SetExplicit.
    _isExplicit = true;
    _SetExplicit(false);
}

template <typename T>
void
SdfListOp<T>::ClearAndMakeExplicit()
{
    // _SetExplicit will clear all items and set the explicit flag as specified.
    // Temporarily change explicit flag to bypass check in _SetExplicit.
    _isExplicit = false;
    _SetExplicit(true);
}

template <typename T>
void 
SdfListOp<T>::ApplyOperations(ItemVector* vec, const ApplyCallback& cb) const
{
    if (!vec) {
        return;
    }

    TRACE_FUNCTION();

    // Apply edits.
    // Note that our use of _ApplyMap in the helper functions below winds up
    // quietly ensuring duplicate items aren't processed in the ItemVector.
    _ApplyList result;
    if (IsExplicit()) {
        _ApplyMap search;
        _AddKeys(SdfListOpTypeExplicit, cb, &result, &search);
    }
    else {
        size_t numToDelete = _deletedItems.size();
        size_t numToAdd = _addedItems.size();
        size_t numToPrepend = _prependedItems.size();
        size_t numToAppend = _appendedItems.size();
        size_t numToOrder = _orderedItems.size();

        if (!cb &&
            ((numToDelete+numToAdd+numToPrepend+numToAppend+numToOrder) == 0)) {
            // nothing to do, so avoid copying vectors
            return;
        }

        // Make a list of the inputs.  We can efficiently (O(1)) splice
        // these elements later.
        result.insert(result.end(), vec->begin(), vec->end());

        // Make a map of keys to list iterators.  This avoids O(n)
        // searches within O(n) loops below.
        _ApplyMap search;
        for (typename _ApplyList::iterator i = result.begin();
             i != result.end(); ++i) {
            search[*i] = i;
        }

        _DeleteKeys (cb, &result, &search);
        _AddKeys(SdfListOpTypeAdded, cb, &result, &search);
        _PrependKeys(cb, &result, &search);
        _AppendKeys(cb, &result, &search);
        _ReorderKeys(cb, &result, &search);
    }

    // Copy the result back to vec.
    vec->clear();
    vec->insert(vec->end(), result.begin(), result.end());
}

template <typename T>
std::optional<SdfListOp<T>>
SdfListOp<T>::ApplyOperations(const SdfListOp<T> &inner) const
{
    if (IsExplicit()) {
        // Explicit list-op replaces the result entirely.
        return *this;
    }
    if (GetAddedItems().empty() && GetOrderedItems().empty()) {
        if (inner.IsExplicit()) {
            ItemVector items = inner.GetExplicitItems();
            ApplyOperations(&items);
            SdfListOp<T> r;
            r.SetExplicitItems(items);
            return r;
        }
        if (inner.GetAddedItems().empty() &&
            inner.GetOrderedItems().empty()) {

             ItemVector del = inner.GetDeletedItems();
             ItemVector pre = inner.GetPrependedItems();
             ItemVector app = inner.GetAppendedItems();

             // Apply deletes
             for (const auto &x: GetDeletedItems()) {
                pre.erase(std::remove(pre.begin(), pre.end(), x), pre.end());
                app.erase(std::remove(app.begin(), app.end(), x), app.end());
                if (std::find(del.begin(), del.end(), x) == del.end()) {
                    del.push_back(x);
                }
             }
             // Apply prepends
             for (const auto &x: GetPrependedItems()) {
                del.erase(std::remove(del.begin(), del.end(), x), del.end());
                pre.erase(std::remove(pre.begin(), pre.end(), x), pre.end());
                app.erase(std::remove(app.begin(), app.end(), x), app.end());
             }
             pre.insert(pre.begin(),
                        GetPrependedItems().begin(),
                        GetPrependedItems().end());
             // Apply appends
             for (const auto &x: GetAppendedItems()) {
                del.erase(std::remove(del.begin(), del.end(), x), del.end());
                pre.erase(std::remove(pre.begin(), pre.end(), x), pre.end());
                app.erase(std::remove(app.begin(), app.end(), x), app.end());
             }
             app.insert(app.end(),
                        GetAppendedItems().begin(),
                        GetAppendedItems().end());

            SdfListOp<T> r;
            r.SetDeletedItems(del);
            r.SetPrependedItems(pre);
            r.SetAppendedItems(app);
            return r;
        }
    }

    // The result is not well-defined, in general.  There is no way
    // to express the combined result as a single SdfListOp.
    //
    // Example for ordered items:
    // - let A have ordered items [2,0]
    // - let B have ordered items [0,1,2]
    // then
    // - A over B over [2,1  ] -> [1,2  ]
    // - A over B over [2,1,0] -> [2,0,1]
    // and there is no way to express the relative order dependency
    // between 1 and 2.
    //
    // Example for added items:
    // - let A have added items [0]
    // - let B have appended items [1] 
    // then
    // - A over B over [   ] -> [1,0]
    // - A over B over [0,1] -> [0,1]
    // and there is no way to express the relative order dependency
    // between 0 and 1.
    //
    return std::optional<SdfListOp<T>>();
}

template <class ItemType, class ListType, class MapType>
static inline
void _InsertIfUnique(const ItemType& item, ListType* result, MapType* search)
{
    if (search->find(item) == search->end()) {
        (*search)[item] = result->insert(result->end(), item);
    }
}

template <class ItemType, class ListType, class MapType>
static inline
void _InsertOrMove(const ItemType& item, typename ListType::iterator pos,
                   ListType* result, MapType* search)
{
    typename MapType::iterator entry = search->find(item);
    if (entry == search->end()) {
        (*search)[item] = result->insert(pos, item);
    } else if (entry->second != pos) {
        result->splice(pos, *result, entry->second, std::next(entry->second));
    }
}

template <class ItemType, class ListType, class MapType>
static inline
void _RemoveIfPresent(const ItemType& item, ListType* result, MapType* search)
{
    typename MapType::iterator j = search->find(item);
    if (j != search->end()) {
        result->erase(j->second);
        search->erase(j);
    }
}

template <class T>
void
SdfListOp<T>::_AddKeys(
    SdfListOpType op,
    const ApplyCallback& callback,
    _ApplyList* result,
    _ApplyMap* search) const
{
    TF_FOR_ALL(i, GetItems(op)) {
        if (callback) {
            if (std::optional<T> item = callback(op, *i)) {
                // Only append if the item isn't already present.
                _InsertIfUnique(*item, result, search);
            }
        }
        else {
            _InsertIfUnique(*i, result, search);
        }
    }
}

template <class T>
void
SdfListOp<T>::_PrependKeys(
    const ApplyCallback& callback,
    _ApplyList* result,
    _ApplyMap* search) const
{
    const ItemVector& items = GetItems(SdfListOpTypePrepended);
    if (callback) {
        for (auto i = items.rbegin(), iEnd = items.rend(); i != iEnd; ++i) {
            if (std::optional<T> mappedItem = callback(SdfListOpTypePrepended, *i)) {
                _InsertOrMove(*mappedItem, result->begin(), result, search);
            }
        }
    } else {
        for (auto i = items.rbegin(), iEnd = items.rend(); i != iEnd; ++i) {
            _InsertOrMove(*i, result->begin(), result, search);
        }
    }
}

template <class T>
void
SdfListOp<T>::_AppendKeys(
    const ApplyCallback& callback,
    _ApplyList* result,
    _ApplyMap* search) const
{
    const ItemVector& items = GetItems(SdfListOpTypeAppended);
    if (callback) {
        for (const T& item: items) {
            if (std::optional<T> mappedItem = callback(SdfListOpTypeAppended, item)) {
                _InsertOrMove(*mappedItem, result->end(), result, search);
            }
        }
    } else {
        for (const T& item: items) {
            _InsertOrMove(item, result->end(), result, search);
        }
    }
}

template <class T>
void
SdfListOp<T>::_DeleteKeys(
    const ApplyCallback& callback,
    _ApplyList* result,
    _ApplyMap* search) const
{
    TF_FOR_ALL(i, GetItems(SdfListOpTypeDeleted)) {
        if (callback) {
            if (std::optional<T> item = callback(SdfListOpTypeDeleted, *i)) {
                _RemoveIfPresent(*item, result, search);
            }
        }
        else {
            _RemoveIfPresent(*i, result, search);
        }
    }
}

template <class T>
void
SdfListOp<T>::_ReorderKeys(
    const ApplyCallback& callback,
    _ApplyList* result,
    _ApplyMap* search) const
{
    _ReorderKeysHelper(GetItems(SdfListOpTypeOrdered), callback, result, search);
}

template <class T>
void
SdfListOp<T>::_ReorderKeysHelper(ItemVector order, const ApplyCallback& callback,
    _ApplyList* result, _ApplyMap* search) {
    
    // Make a vector and set of the source items.
    ItemVector uniqueOrder;
    std::set<ItemType, _ItemComparator> orderSet;
    TF_FOR_ALL(i, order) {
        if (callback) {
            if (std::optional<T> item = callback(SdfListOpTypeOrdered, *i)) {
                if (orderSet.insert(*item).second) {
                    uniqueOrder.push_back(*item);
                }
            }
        }
        else {
            if (orderSet.insert(*i).second) {
                uniqueOrder.push_back(*i);
            }
        }
    }
    if (uniqueOrder.empty()) {
        return;
    }

    // Move the result aside for now.
    _ApplyList scratch;
    std::swap(scratch, *result);

    // Find each item from the order vector in the scratch list.
    // Then find the next item in the scratch list that's also in
    // in the uniqueOrder vector.  All of these items except the last
    // form the next continuous sequence in the result.
    TF_FOR_ALL(i, uniqueOrder) {
        typename _ApplyMap::const_iterator j = search->find(*i);
        if (j != search->end()) {
            // Find the next item in both scratch and order.
            typename _ApplyList::iterator e = j->second;
            do {
                ++e;
            } while (e != scratch.end() && orderSet.count(*e) == 0);

            // Move the sequence to result.
            result->splice(result->end(), scratch, j->second, e);
        }
    }

    // Any items remaining in scratch are neither in order nor after
    // anything in order.  Therefore they must be first in their
    // current order.
    result->splice(result->begin(), scratch);
}

template <typename T>
static inline
bool
_ModifyCallbackHelper(const typename SdfListOp<T>::ModifyCallback& cb,
                      std::vector<T>* itemVector)
{
    bool didModify = false;

    std::vector<T> modifiedVector;
    modifiedVector.reserve(itemVector->size());
    TfDenseHashSet<T, TfHash> existingSet;

    for (const T& item : *itemVector) {
        std::optional<T> modifiedItem = cb(item);
        if (modifiedItem) {
            if (!existingSet.insert(*modifiedItem).second) {
                modifiedItem = std::nullopt;
            }
        }

        if (!modifiedItem) {
            didModify = true;
        }
        else if (*modifiedItem != item) {
            modifiedVector.push_back(std::move(*modifiedItem));
            didModify = true;
        } else {
            modifiedVector.push_back(item);
        }
    }

    if (didModify) {
        itemVector->swap(modifiedVector);
    }

    return didModify;
}

template <typename T>
bool 
SdfListOp<T>::ModifyOperations(const ModifyCallback& callback)
{
    bool didModify = false;

    if (callback) {
        didModify |= _ModifyCallbackHelper(
            callback, &_explicitItems);
        didModify |= _ModifyCallbackHelper(
            callback, &_addedItems);
        didModify |= _ModifyCallbackHelper(
            callback, &_prependedItems);
        didModify |= _ModifyCallbackHelper(
            callback, &_appendedItems);
        didModify |= _ModifyCallbackHelper(
            callback, &_deletedItems);
        didModify |= _ModifyCallbackHelper(
            callback, &_orderedItems);
    }

    return didModify;
}

template <typename T>
bool 
SdfListOp<T>::ModifyOperations(const ModifyCallback& callback,
                               bool unusedRemoveDuplicates)
{
    return ModifyOperations(callback);
}

template <typename T>
bool 
SdfListOp<T>::ReplaceOperations(const SdfListOpType op, size_t index, size_t n, 
                               const ItemVector& newItems)
{
    bool needsModeSwitch = 
        (IsExplicit() && op != SdfListOpTypeExplicit) ||
        (!IsExplicit() && op == SdfListOpTypeExplicit);

    // XXX: This behavior was copied from GdListEditor, which
    //      appears to have been copied from old Sd code...
    //      the circle is now complete.
    //
    // XXX: This is to mimic old Sd list editor behavior.  If
    //      we insert into a list we should automatically change
    //      modes, but if we replace or remove then we should
    //      silently ignore the request.
    if (needsModeSwitch && (n > 0 || newItems.empty())) {
        return false;
    }

    ItemVector itemVector = GetItems(op);

    if (index > itemVector.size()) {
        TF_CODING_ERROR("Invalid start index %zd (size is %zd)",
                        index, itemVector.size());
        return false;
    }
    else if (index + n > itemVector.size()) {
        TF_CODING_ERROR("Invalid end index %zd (size is %zd)",
                        index + n - 1, itemVector.size());
        return false;
    }

    if (n == newItems.size()) {
        std::copy(newItems.begin(), newItems.end(), itemVector.begin() + index);
    }
    else {
        itemVector.erase(itemVector.begin() + index, 
                         itemVector.begin() + index + n);
        itemVector.insert(itemVector.begin() + index,
                          newItems.begin(), newItems.end());
    }

    SetItems(itemVector, op);
    return true;
}

template <typename T>
void 
SdfListOp<T>::ComposeOperations(const SdfListOp<T>& stronger, SdfListOpType op)
{

    SdfListOp<T> &weaker = *this;

    if (op == SdfListOpTypeExplicit) {
        weaker.SetItems(stronger.GetItems(op), op);
    }
    else {
        const ItemVector &weakerVector = weaker.GetItems(op);
        _ApplyList weakerList(weakerVector.begin(), weakerVector.end());
        _ApplyMap weakerSearch;
        for (typename _ApplyList::iterator i = weakerList.begin(); 
                i != weakerList.end(); ++i) {
            weakerSearch[*i] = i;
        }

        if (op == SdfListOpTypeOrdered) {
            stronger._AddKeys(op, ApplyCallback(), 
                                 &weakerList, &weakerSearch);
            stronger._ReorderKeys(ApplyCallback(), 
                                  &weakerList, &weakerSearch);
        } else if (op == SdfListOpTypeAdded) {
            stronger._AddKeys(op,
                                 ApplyCallback(),
                                 &weakerList,
                                 &weakerSearch);
        } else if (op == SdfListOpTypeDeleted) {
            stronger._AddKeys(op,
                                 ApplyCallback(),
                                 &weakerList,
                                 &weakerSearch);
        } else if (op == SdfListOpTypePrepended) {
            stronger._PrependKeys(ApplyCallback(),
                                 &weakerList,
                                 &weakerSearch);
        } else if (op == SdfListOpTypeAppended) {
            stronger._AppendKeys(ApplyCallback(),
                                 &weakerList,
                                 &weakerSearch);
        }

        weaker.SetItems(ItemVector(weakerList.begin(), weakerList.end()), op);
    }
}

////////////////////////////////////////////////////////////////////////
// Free functions

template <class ItemType>
void SdfApplyListOrdering(std::vector<ItemType>* v, 
                         const std::vector<ItemType>& order)
{
    if (!order.empty() && !v->empty()) {
        // Make a list of the inputs.  We can efficiently (O(1)) splice
        // these elements later.
        typename SdfListOp<ItemType>::_ApplyList result;
        result.insert(result.end(), v->begin(), v->end());

        // Make a map of keys to list iterators.  This avoids O(n)
        // searches within O(n) loops below.
        typename SdfListOp<ItemType>::_ApplyMap search;
        typename SdfListOp<ItemType>::_ApplyList::iterator i = result.begin();
        for (;
             i != result.end(); ++i) {
            search[*i] = i;
        }
        SdfListOp<ItemType>::_ReorderKeysHelper(order, nullptr, &result, &search);

        // Copy the result back to vec.
        v->clear();
        v->insert(v->end(), result.begin(), result.end());
    }
}

////////////////////////////////////////////////////////////////////////
// Stream i/o

template <typename T>
static void
_StreamOutItems(
    std::ostream &out,
    const string &itemsName,
    const std::vector<T> &items,
    bool *firstItems,
    bool isExplicitList = false)
{
    if (isExplicitList || !items.empty()) {
        out << (*firstItems ? "" : ", ") << itemsName << " Items: [";
        *firstItems = false;
        TF_FOR_ALL(it, items) {
            out << *it << (it.GetNext() ? ", " : "");
        }
        out << "]";
    }
}

template <typename T>
static std::ostream &
_StreamOut(std::ostream &out, const SdfListOp<T> &op)
{
    const std::vector<std::string>& listOpAliases = 
        TfType::GetRoot().GetAliases(TfType::Find<SdfListOp<T>>());
    TF_VERIFY(!listOpAliases.empty());

    out << listOpAliases.front() << "(";
    bool firstItems = true;
    if (op.IsExplicit()) {
        _StreamOutItems(
            out, "Explicit", op.GetExplicitItems(), &firstItems,
            /* isExplicitList = */ true);
    }
    else {
        _StreamOutItems(out, "Deleted", op.GetDeletedItems(), &firstItems);
        _StreamOutItems(out, "Added", op.GetAddedItems(), &firstItems);
        _StreamOutItems(out, "Prepended", op.GetPrependedItems(), &firstItems);
        _StreamOutItems(out, "Appended", op.GetAppendedItems(), &firstItems);
        _StreamOutItems(out, "Ordered", op.GetOrderedItems(), &firstItems);
    }
    out << ")";
    return out;
}

template <typename ITEM_TYPE>
std::ostream & operator<<( std::ostream &out, const SdfListOp<ITEM_TYPE> &op)
{
    return _StreamOut(out, op);
}

////////////////////////////////////////////////////////////////////////
// Traits

template<>
struct Sdf_ListOpTraits<TfToken>
{
    typedef TfTokenFastArbitraryLessThan ItemComparator;
};

template<>
struct Sdf_ListOpTraits<SdfPath>
{
    typedef SdfPath::FastLessThan ItemComparator;
};

template<>
struct Sdf_ListOpTraits<SdfUnregisteredValue>
{
    struct LessThan {
        bool operator()(const SdfUnregisteredValue& x,
                        const SdfUnregisteredValue& y) const {
            const size_t xHash = hash_value(x);
            const size_t yHash = hash_value(y);
            if (xHash < yHash) {
                return true;
            }
            else if (xHash > yHash || x == y) {
                return false;
            }

            // Fall back to comparing the string representations if
            // the hashes of x and y are equal but x and y are not.
            return TfStringify(x) < TfStringify(y);
        }
    };

    typedef LessThan ItemComparator;
};

////////////////////////////////////////////////////////////////////////
// Template instantiation.

#define SDF_INSTANTIATE_LIST_OP(ValueType)                       \
    template class SdfListOp<ValueType>;                         \
    template SDF_API std::ostream&                               \
    operator<<(std::ostream &, const SdfListOp<ValueType> &)     \

SDF_INSTANTIATE_LIST_OP(int);
SDF_INSTANTIATE_LIST_OP(unsigned int);
SDF_INSTANTIATE_LIST_OP(int64_t);
SDF_INSTANTIATE_LIST_OP(uint64_t);
SDF_INSTANTIATE_LIST_OP(string);
SDF_INSTANTIATE_LIST_OP(TfToken);
SDF_INSTANTIATE_LIST_OP(SdfUnregisteredValue);
SDF_INSTANTIATE_LIST_OP(SdfPath);
SDF_INSTANTIATE_LIST_OP(SdfReference);
SDF_INSTANTIATE_LIST_OP(SdfPayload);

template
SDF_API void SdfApplyListOrdering(std::vector<string>* v, 
                          const std::vector<string>& order);
template
SDF_API void SdfApplyListOrdering(std::vector<TfToken>* v, 
                          const std::vector<TfToken>& order);

}  // namespace pxr
