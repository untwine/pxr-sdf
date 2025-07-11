// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./propertySpec.h"
#include "./accessorHelpers.h"
#include "./childrenUtils.h"
#include "./layer.h"
#include "./pathExpression.h"
#include "./primSpec.h"
#include "./schema.h"

#include <pxr/tf/iterator.h>
#include <pxr/tf/staticData.h>

#include <pxr/plug/registry.h>
#include <pxr/plug/plugin.h>
#include <pxr/trace/trace.h>

#include <ostream>

namespace pxr {

SDF_DEFINE_ABSTRACT_SPEC(SdfSchema, SdfPropertySpec, SdfSpec);

//
// Name
//

const std::string &
SdfPropertySpec::GetName() const
{
    return GetPath().GetName();
}

TfToken
SdfPropertySpec::GetNameToken() const
{
    return GetPath().GetNameToken();
}

bool
SdfPropertySpec::CanSetName(const std::string &newName,
                               std::string *whyNot) const
{
    return Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>::CanRename(
        *this, TfToken(newName)).IsAllowed(whyNot);
}

bool
SdfPropertySpec::SetName(const std::string &newName,
                        bool validate)
{
    return Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>::Rename(
        *this, TfToken(newName));
}

bool
SdfPropertySpec::IsValidName(const std::string &name)
{
    return Sdf_ChildrenUtils<Sdf_PropertyChildPolicy>::IsValidName(name);
}

//
// Ownership
//

SdfSpecHandle
SdfPropertySpec::GetOwner() const
{
    SdfPath parentPath = GetPath().GetParentPath();

    // If this spec is a relational attribute, its parent path will be
    // a target path. Since Sdf does not provide specs for relationship targets
    // we return the target's owning relationship instead.
    if (parentPath.IsTargetPath()) {
        parentPath = parentPath.GetParentPath();
    }
    
    return GetLayer()->GetObjectAtPath(parentPath);
}

//
// Metadata, Property Value API, and Spec Properties
// (methods built on generic SdfSpec accessor macros)
//

// Initialize accessor helper macros to associate with this class and optimize
// out the access predicate
#define SDF_ACCESSOR_CLASS                   SdfPropertySpec
#define SDF_ACCESSOR_READ_PREDICATE(key_)    SDF_NO_PREDICATE
#define SDF_ACCESSOR_WRITE_PREDICATE(key_)   SDF_NO_PREDICATE

// Metadata
SDF_DEFINE_GET_SET(DisplayGroup,     SdfFieldKeys->DisplayGroup,     std::string)
SDF_DEFINE_GET_SET(DisplayName,      SdfFieldKeys->DisplayName,      std::string)
SDF_DEFINE_GET_SET(Documentation,    SdfFieldKeys->Documentation,    std::string)
SDF_DEFINE_GET_SET(Hidden,           SdfFieldKeys->Hidden,           bool)
SDF_DEFINE_GET_SET(Prefix,           SdfFieldKeys->Prefix,           std::string)
SDF_DEFINE_GET_SET(Suffix,           SdfFieldKeys->Suffix,           std::string)
SDF_DEFINE_GET_SET(SymmetricPeer,    SdfFieldKeys->SymmetricPeer,    std::string)
SDF_DEFINE_GET_SET(SymmetryFunction, SdfFieldKeys->SymmetryFunction, TfToken)

SDF_DEFINE_TYPED_GET_SET(Permission, SdfFieldKeys->Permission, 
                        SdfPermission, SdfPermission)

SDF_DEFINE_DICTIONARY_GET_SET(GetCustomData, SetCustomData,
                             SdfFieldKeys->CustomData);
SDF_DEFINE_DICTIONARY_GET_SET(GetSymmetryArguments, SetSymmetryArgument,
                             SdfFieldKeys->SymmetryArguments);
SDF_DEFINE_DICTIONARY_GET_SET(GetAssetInfo, SetAssetInfo,
                             SdfFieldKeys->AssetInfo);

// Property Value API
// Note: Default value is split up into individual macro calls as the Set
//       requires a boolean return and there's no more-convenient way to
//       shanghai the accessor macros to provide that generically.
SDF_DEFINE_GET(DefaultValue,   SdfFieldKeys->Default, VtValue)
SDF_DEFINE_HAS(DefaultValue,   SdfFieldKeys->Default)
SDF_DEFINE_CLEAR(DefaultValue, SdfFieldKeys->Default)

// Spec Properties
SDF_DEFINE_IS_SET(Custom, SdfFieldKeys->Custom)

SDF_DEFINE_GET_SET(Comment, SdfFieldKeys->Comment, std::string)

SDF_DEFINE_GET(Variability, SdfFieldKeys->Variability, SdfVariability)

// See comment in GetTypeName()
SDF_DEFINE_GET_PRIVATE(AttributeValueTypeName, SdfFieldKeys->TypeName, TfToken)

// Clean up macro shenanigans
#undef SDF_ACCESSOR_CLASS
#undef SDF_ACCESSOR_READ_PREDICATE
#undef SDF_ACCESSOR_WRITE_PREDICATE

//
// Metadata, Property Value API, and Spec Properties
// (methods requiring additional logic)
//

bool
SdfPropertySpec::SetDefaultValue(const VtValue &defaultValue)
{
    if (defaultValue.IsEmpty()) {
        ClearDefaultValue();
        return true;
    }

    TfType valueType = GetValueType();
    if (valueType.IsUnknown()) {
        if (defaultValue.IsHolding<SdfValueBlock>()) {
            // Allow blocking unknown types.
            return SetField(SdfFieldKeys->Default, defaultValue);
        }
        TF_CODING_ERROR("Can't set value on attribute <%s> with "
                        "unknown type \"%s\"",
                        GetPath().GetText(),
                        GetTypeName().GetAsToken().GetText());
        return false;
    }
    static const TfType opaqueType = TfType::Find<SdfOpaqueValue>();
    if (valueType == opaqueType) {
        TF_CODING_ERROR("Can't set value on <%s>: %s-typed attributes "
                        "cannot have an authored default value",
                        GetPath().GetAsString().c_str(),
                        GetTypeName().GetAsToken().GetText());
        return false;
    }

    // valueType may be an enum type provided by a plugin which has not been
    // loaded.
    if (valueType.GetTypeid() == typeid(void) || valueType.IsEnumType()) {
        // If we are dealing with an enum then we just make sure the TfTypes
        // match up. Authoring integral values to enum typed properties is
        // disallowed.
        if (valueType == defaultValue.GetType()) {
            return SetField(SdfFieldKeys->Default, defaultValue);
        }
    }
    else {
        // Otherwise check if defaultValue is castable to valueType
        VtValue value =
            VtValue::CastToTypeid(defaultValue, valueType.GetTypeid());
        if (!value.IsEmpty()) {
            // If this value is a pathExpression, make all embedded paths
            // absolute using this property's prim path as the anchor.
            if (value.IsHolding<SdfPathExpression>() &&
                !value.UncheckedGet<SdfPathExpression>().IsAbsolute()) {
                value.UncheckedMutate<SdfPathExpression>(
                    [&](SdfPathExpression &expr) {
                        expr = expr.MakeAbsolute(GetPath().GetPrimPath());
                    });
            }
            else if (value.IsHolding<VtArray<SdfPathExpression>>()) {
                SdfPath const &anchor = GetPath().GetPrimPath();
                value.UncheckedMutate<VtArray<SdfPathExpression>>(
                    [&](VtArray<SdfPathExpression> &exprArr) {
                        for (SdfPathExpression &expr: exprArr) {
                            expr = expr.MakeAbsolute(anchor);
                        }
                    });
            }
            /*
            // If this value is a path (relationship default-values are paths),
            // make it absolute using this property's prim path as the anchor.
            else if (value.IsHolding<SdfPath>() &&
                     !value.UncheckedGet<SdfPath>().IsAbsolutePath()) {
                value.UncheckedMutate<SdfPath>([&](SdfPath &path) {
                    path = path.MakeAbsolutePath(GetPath().GetPrimPath());
                });
            }
            */
            return SetField(SdfFieldKeys->Default, value);
        }
        else if (defaultValue.IsHolding<SdfValueBlock>()) {
            // If we're setting a value block, always allow that.
            return SetField(SdfFieldKeys->Default, defaultValue);
        }
    }

    // If we reach here, we are either assigning invalid values to enum types
    // or defaultValue can't cast to valueType.
    TF_CODING_ERROR("Can't set value on <%s> to %s: "
                    "expected a value of type \"%s\"",
                    GetPath().GetText(),
                    TfStringify(defaultValue).c_str(),
                    valueType.GetTypeName().c_str());
    return false;
}

TfType
SdfPropertySpec::GetValueType() const
{
    // The value type of an attribute is specified by the user when it is
    // constructed, while the value type of a relationship is always SdfPath.
    // Normally, one would use virtual functions to encapsulate this difference;
    // however we don't want to use virtuals as SdfSpec and its subclasses are 
    // intended to be simple value types that are merely wrappers around
    // a layer. So, we have this hacky 'virtual' function.
    switch (GetSpecType()) {
    case SdfSpecTypeAttribute:
        return GetSchema().FindType(_GetAttributeValueTypeName()).GetType();

    case SdfSpecTypeRelationship: {
        static const TfType type = TfType::Find<SdfPath>();
        return type;
    }

    default:
        TF_CODING_ERROR("Unrecognized subclass of SdfPropertySpec on <%s>",
                        GetPath().GetText());
        return TfType();
    }
}

SdfValueTypeName
SdfPropertySpec::GetTypeName() const
{
    // See comment in GetValueType().
    switch (GetSpecType()) {
    case SdfSpecTypeAttribute:
        return GetSchema().FindOrCreateType(_GetAttributeValueTypeName());

    case SdfSpecTypeRelationship:
        return SdfValueTypeName();

    default:
        TF_CODING_ERROR("Unrecognized subclass of SdfPropertySpec on <%s>",
                        GetPath().GetText());
        return SdfValueTypeName();
    }
}

bool
SdfPropertySpec::HasOnlyRequiredFields() const
{
    return GetLayer()->_IsInert(GetPath(), true /*ignoreChildren*/, 
                       true /* requiredFieldOnlyPropertiesAreInert */);
}

}  // namespace pxr
