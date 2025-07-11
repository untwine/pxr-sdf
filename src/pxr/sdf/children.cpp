// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./children.h"
#include "./attributeSpec.h"
#include "./childrenPolicies.h"
#include "./childrenUtils.h"
#include "./layer.h"
#include "./primSpec.h"
#include "./propertySpec.h"
#include "./relationshipSpec.h"
#include "./variantSetSpec.h"
#include "./variantSpec.h"
#include <pxr/tf/ostreamMethods.h>

namespace pxr {

template<class ChildPolicy>
Sdf_Children<ChildPolicy>::Sdf_Children() :
    _childNamesValid(false)
{
}

template<class ChildPolicy>
Sdf_Children<ChildPolicy>::Sdf_Children(const Sdf_Children<ChildPolicy> &other) :
    _layer(other._layer),
    _parentPath(other._parentPath),
    _childrenKey(other._childrenKey),
    _keyPolicy(other._keyPolicy),
    _childNamesValid(false)
{
}

template<class ChildPolicy>
Sdf_Children<ChildPolicy>::Sdf_Children(const SdfLayerHandle &layer,
    const SdfPath &parentPath, const TfToken &childrenKey,
    const KeyPolicy& keyPolicy) : 
    _layer(layer),
    _parentPath(parentPath),
    _childrenKey(childrenKey),
    _keyPolicy(keyPolicy),
    _childNamesValid(false)
{
}

template<class ChildPolicy>
size_t
Sdf_Children<ChildPolicy>::GetSize() const
{
    _UpdateChildNames();
        
    return _childNames.size();
}

template<class ChildPolicy>
typename Sdf_Children<ChildPolicy>::ValueType
Sdf_Children<ChildPolicy>::GetChild(size_t index) const
{
    if (!TF_VERIFY(IsValid())) {
        return ValueType();
    }

    _UpdateChildNames();
        
    // XXX: Would like to avoid unnecessary dynamic_casts...
    SdfPath childPath = 
        ChildPolicy::GetChildPath(_parentPath, _childNames[index]);
    return TfDynamic_cast<ValueType>(_layer->GetObjectAtPath(childPath));
}

template<class ChildPolicy>
bool
Sdf_Children<ChildPolicy>::IsValid() const
{
    // XXX: Should we also check for the existence of the spec?
    return _layer && !_parentPath.IsEmpty();
}

template<class ChildPolicy>
size_t
Sdf_Children<ChildPolicy>::Find(const KeyType &key) const
{
    if (!TF_VERIFY(IsValid())) {
        return 0;
    }

    _UpdateChildNames();

    const FieldType expectedKey(_keyPolicy.Canonicalize(key));
    size_t i = 0;
    for (i=0; i < _childNames.size(); i++) {
        if (_childNames[i] == expectedKey) {
            break;
        }
    }
    return i;
}

template<class ChildPolicy>
typename Sdf_Children<ChildPolicy>::KeyType
Sdf_Children<ChildPolicy>::FindKey(const ValueType &x) const
{
    if (!TF_VERIFY(IsValid())) {
        return KeyType();
    }

    // If the value is invalid or does not belong to this layer,
    // then return a default-constructed key.
    if (!x || x->GetLayer() != _layer) {
        return KeyType();
    }

    // If the value's path is not a child path of the parent path,
    // then return a default-constructed key.
    if (ChildPolicy::GetParentPath(x->GetPath()) != _parentPath) {
        return KeyType();
    }
        
    return ChildPolicy::GetKey(x);
}

template<class ChildPolicy>
bool
Sdf_Children<ChildPolicy>::IsEqualTo(const Sdf_Children<ChildPolicy> &other) const
{
    // Return true if this and other refer to the same set of children
    // on the same object in the same layer.
    return (_layer == other._layer && _parentPath == other._parentPath &&
        _childrenKey == other._childrenKey);
}

template<class ChildPolicy>
bool
Sdf_Children<ChildPolicy>::Copy(
    const std::vector<ValueType> & values,
    const std::string &type)
{
    _childNamesValid = false;

    if (!TF_VERIFY(IsValid())) {
        return false;
    }

    return Sdf_ChildrenUtils<ChildPolicy>::SetChildren(
        _layer, _parentPath, values);
}

template<class ChildPolicy>
bool
Sdf_Children<ChildPolicy>::Insert(const ValueType& value, size_t index, const std::string &type)
{
    _childNamesValid = false;

    if (!TF_VERIFY(IsValid())) {
        return false;
    }

    return Sdf_ChildrenUtils<ChildPolicy>::InsertChild(
        _layer, _parentPath, value, index);
}

template<class ChildPolicy>
bool
Sdf_Children<ChildPolicy>::Erase(const KeyType& key, const std::string &type)
{
    _childNamesValid = false;

    if (!TF_VERIFY(IsValid())) {
        return false;
    }

    const FieldType expectedKey(_keyPolicy.Canonicalize(key));

    return Sdf_ChildrenUtils<ChildPolicy>::RemoveChild(
        _layer, _parentPath, expectedKey);
}

template<class ChildPolicy>
void
Sdf_Children<ChildPolicy>::_UpdateChildNames() const
{
    if (_childNamesValid) {
        return;
    }
    _childNamesValid = true;

    if (_layer) {
        _childNames = _layer->template GetFieldAs<std::vector<FieldType>>(
            _parentPath, _childrenKey);
    } else {
        _childNames.clear();
    }
}

template class Sdf_Children<Sdf_AttributeChildPolicy>;
template class Sdf_Children<Sdf_MapperChildPolicy>;
template class Sdf_Children<Sdf_MapperArgChildPolicy>;
template class Sdf_Children<Sdf_PrimChildPolicy>;
template class Sdf_Children<Sdf_PropertyChildPolicy>;
template class Sdf_Children<Sdf_RelationshipChildPolicy>;
template class Sdf_Children<Sdf_VariantChildPolicy>;
template class Sdf_Children<Sdf_VariantSetChildPolicy>;

}  // namespace pxr
