// Copyright 2017 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./copyUtils.h"

#include "./changeBlock.h"
#include "./childrenPolicies.h"
#include "./childrenUtils.h"
#include "./layer.h"
#include "./path.h"
#include "./payload.h"
#include "./primSpec.h"
#include "./reference.h"

#include <pxr/tf/token.h>
#include <pxr/vt/value.h>

#include <deque>
#include <utility>
#include <vector>

namespace pxr {

namespace {

    // A _CopyStackEntry is a (source path, destination path) pair indicating
    // a spec that should be copied.
    struct _CopyStackEntry {
        SdfPath srcPath;
        SdfPath dstPath;

        _CopyStackEntry(const SdfPath& srcPath, const SdfPath& dstPath) :
            srcPath(srcPath), dstPath(dstPath) { };
    };

    typedef std::deque<_CopyStackEntry> _CopyStack;

    // Collection of (field name, value) pairs.
    typedef std::pair<TfToken, VtValue> _FieldValuePair;
    typedef std::vector<_FieldValuePair> _FieldValueList;

    // A _SpecDataEntry contains all of the information being copied for a 
    // single spec.
    struct _SpecDataEntry {
        _SpecDataEntry(const SdfPath& dstPath_, SdfSpecType specType_)
            : dstPath(dstPath_), specType(specType_) { }
        
        // Destination path to which this spec data should be copied.
        SdfPath dstPath;

        // Type of spec this entry represents.
        SdfSpecType specType;

        // List containing (field, value) pairs of data to be copied to
        // the destination spec.
        _FieldValueList dataToCopy;
    };

} // end anonymous namespace

// Helper to get sorted, filtered field names.
template <class SchemaPred>
static TfTokenVector
_GetFilteredFieldNames(const SdfLayerHandle &layer,
                       const SdfPath &path,
                       SchemaPred const &pred)
{
    const SdfSchemaBase &schema = layer->GetSchema();
    std::vector<TfToken> fieldNames = layer->ListFields(path);
    fieldNames.erase(std::remove_if(fieldNames.begin(), fieldNames.end(),
                                    [&](const TfToken &fieldName) {
                                        return !pred(schema, fieldName);
                                    }), fieldNames.end());
    TfTokenFastArbitraryLessThan lessThan;
    std::sort(fieldNames.begin(), fieldNames.end(), lessThan);
    return fieldNames;
}

// Returns lists of value field names to be handled during
// the copy process. The returned lists are sorted using the
// TfTokenFastArbitraryLessThan comparator.
static TfTokenVector
_GetValueFieldNames(const SdfLayerHandle& layer, const SdfPath& path)
{
    return _GetFilteredFieldNames(
        layer, path,
        [](const SdfSchemaBase &schema, const TfToken &fieldName) {
            return !schema.HoldsChildren(fieldName);
        });
}

// Returns lists of children field names to be handled during
// the copy process. The returned lists are sorted using the
// TfTokenFastArbitraryLessThan comparator.
static TfTokenVector
_GetChildrenFieldNames(const SdfLayerHandle& layer, const SdfPath& path)
{
    return _GetFilteredFieldNames(
        layer, path,
        [](const SdfSchemaBase &schema, const TfToken &fieldName) {
            return schema.HoldsChildren(fieldName);
        });
}

// Process the given children and add any children specs that are indicated by
// the copy policy to the list of specs to be copied.
template <class ChildPolicy>
static void
_ProcessChildren(
    const TfToken& childrenField, 
    const VtValue& srcChildrenValue, const VtValue& dstChildrenValue,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool childrenInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool childrenInDst,
    _CopyStack* copyStack)
{
    typedef typename ChildPolicy::FieldType FieldType;
    typedef std::vector<FieldType> ChildrenVector;

    if (!TF_VERIFY(srcChildrenValue.IsHolding<ChildrenVector>() || 
                   srcChildrenValue.IsEmpty()) ||
        !TF_VERIFY(dstChildrenValue.IsHolding<ChildrenVector>() || 
                   dstChildrenValue.IsEmpty())) {
        return;
    }

    const ChildrenVector emptyChildren;
    const ChildrenVector& srcChildren = srcChildrenValue.IsEmpty() ? 
        emptyChildren : srcChildrenValue.UncheckedGet<ChildrenVector>();
    const ChildrenVector& dstChildren = dstChildrenValue.IsEmpty() ? 
        emptyChildren : dstChildrenValue.UncheckedGet<ChildrenVector>();

    for (size_t i = 0; i < srcChildren.size(); ++i) {
        if (srcChildren[i].IsEmpty() || dstChildren[i].IsEmpty()) {
            continue;
        }

        const SdfPath srcChildPath = 
            ChildPolicy::GetChildPath(srcPath, srcChildren[i]);
        const SdfPath dstChildPath = 
            ChildPolicy::GetChildPath(dstPath, dstChildren[i]);

        copyStack->emplace_back(srcChildPath, dstChildPath);
    }

    // Add entries to the copy stack to mark the removal of child specs
    // in the destination layer that aren't included in the list of children
    // to copy.
    if (childrenInDst) {
        const VtValue oldDstChildrenValue = 
            dstLayer->GetField(dstPath, childrenField);
        if (!TF_VERIFY(oldDstChildrenValue.IsHolding<ChildrenVector>())) {
            return;
        }

        for (const auto& oldDstChild : 
                 oldDstChildrenValue.UncheckedGet<ChildrenVector>()) {
            if (std::find(dstChildren.begin(), dstChildren.end(), 
                    oldDstChild) == dstChildren.end()) {
                
                const SdfPath oldDstChildPath = 
                    ChildPolicy::GetChildPath(dstPath, oldDstChild);
                copyStack->emplace_back(SdfPath(), oldDstChildPath);
            }
        }
    }
}

static void
_ProcessChildField(
    const TfToken& childField, 
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool childrenInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool childrenInDst,
    const SdfShouldCopyChildrenFn& shouldCopyChildren, _CopyStack* copyStack)
{
    std::optional<VtValue> srcChildrenToCopy, dstChildrenToCopy;
    if (!shouldCopyChildren(
            childField, 
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            &srcChildrenToCopy, &dstChildrenToCopy)) {
        return;
    }

    if (!srcChildrenToCopy || !dstChildrenToCopy) {
        srcChildrenToCopy = srcLayer->GetField(srcPath, childField);
        dstChildrenToCopy = srcChildrenToCopy;
    }

    const VtValue& srcChildren = *srcChildrenToCopy;
    const VtValue& dstChildren = *dstChildrenToCopy;

    if (childField == SdfChildrenKeys->ConnectionChildren) {
        _ProcessChildren<Sdf_AttributeConnectionChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->MapperChildren) {
        _ProcessChildren<Sdf_MapperChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->MapperArgChildren) {
        _ProcessChildren<Sdf_MapperArgChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->ExpressionChildren) {
        _ProcessChildren<Sdf_ExpressionChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->RelationshipTargetChildren) {
        _ProcessChildren<Sdf_RelationshipTargetChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->VariantChildren) {
        _ProcessChildren<Sdf_VariantChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->VariantSetChildren) {
        _ProcessChildren<Sdf_VariantSetChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->PropertyChildren) {
        _ProcessChildren<Sdf_PropertyChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }
    if (childField == SdfChildrenKeys->PrimChildren) {
        _ProcessChildren<Sdf_PrimChildPolicy>(
            childField, srcChildren, dstChildren,
            srcLayer, srcPath, childrenInSrc, dstLayer, dstPath, childrenInDst,
            copyStack);
        return;
    }

    TF_CODING_ERROR("Unknown child field '%s'", childField.GetText());
}

// Helpers to add a new spec to the given layer.
template <class ChildPolicy>
static void
_DoAddNewSpec(const SdfLayerHandle& destLayer, const _SpecDataEntry& specData)
{
    Sdf_ChildrenUtils<ChildPolicy>::CreateSpec(destLayer, specData.dstPath,
        specData.specType);
}

static
void _DoAddNewPrimSpec(
    const SdfLayerHandle& destLayer, const _SpecDataEntry& specData)
{
    // Need to determine whether this property is considered inert when
    // being initially created based on fields being copied in. This mimics
    // what's done in the SdfPrimSpec constructor.
    TfToken type;
    SdfSpecifier specifier = SdfSpecifierOver;
    for (const _FieldValuePair& fieldValue : specData.dataToCopy) {
        if (fieldValue.second.IsEmpty()) {
            continue;
        }

        if (fieldValue.first == SdfFieldKeys->TypeName) {
            type = fieldValue.second.Get<TfToken>();
        }
        else if (fieldValue.first == SdfFieldKeys->Specifier) {
            specifier = fieldValue.second.Get<SdfSpecifier>();
        }
    }

    const bool inert = (specifier == SdfSpecifierOver && type.IsEmpty());
    Sdf_ChildrenUtils<Sdf_PrimChildPolicy>::CreateSpec(
        destLayer, specData.dstPath, SdfSpecTypePrim,
        /* inert = */ inert);
}

template <class ChildPolicy>
static void
_DoAddNewPropertySpec(
    const SdfLayerHandle& destLayer, const _SpecDataEntry& specData)
{
    // Need to determine whether this property is considered to have only 
    // required fields when being initially created based on fields being 
    // copied in. This mimics what's done in the 
    // SdfAttributeSpec/SdfRelationshipSpec constructors.
    bool custom = false;
    for (const _FieldValuePair& fieldValue : specData.dataToCopy) {
        if (fieldValue.first == SdfFieldKeys->Custom) {
            custom = fieldValue.second.template Get<bool>();
            break;
        }
    }

    const bool hasOnlyRequiredFields = (!custom);
    Sdf_ChildrenUtils<ChildPolicy>::CreateSpec(
        destLayer, specData.dstPath, specData.specType, 
        /* hasOnlyRequiredFields = */ hasOnlyRequiredFields);
}

static void
_AddNewSpecToLayer(
    const SdfLayerHandle& destLayer, const _SpecDataEntry& specData)
{
    if (destLayer->HasSpec(specData.dstPath)) {
        return;
    }

    switch (specData.specType) {
    case SdfSpecTypeAttribute:
        _DoAddNewPropertySpec<Sdf_AttributeChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeConnection:
        _DoAddNewSpec<Sdf_AttributeConnectionChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeExpression:
        _DoAddNewSpec<Sdf_ExpressionChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeMapper:
        _DoAddNewSpec<Sdf_MapperChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeMapperArg:
        _DoAddNewSpec<Sdf_MapperArgChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypePrim:
        _DoAddNewPrimSpec(destLayer, specData);
        break;
    case SdfSpecTypeRelationship:
        _DoAddNewPropertySpec<Sdf_RelationshipChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeRelationshipTarget:
        _DoAddNewSpec<Sdf_RelationshipTargetChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeVariant:
        _DoAddNewSpec<Sdf_VariantChildPolicy>(destLayer, specData);
        break;
    case SdfSpecTypeVariantSet:
        _DoAddNewSpec<Sdf_VariantSetChildPolicy>(destLayer, specData);
        break;

    case SdfSpecTypePseudoRoot:
    case SdfSpecTypeUnknown:
    case SdfNumSpecTypes:
        break;
    }
}

template <class ChildPolicy>
static void
_DoRemoveSpec(
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath)
{
    Sdf_ChildrenUtils<ChildPolicy>::RemoveChild(
        dstLayer, 
        ChildPolicy::GetParentPath(dstPath),
        ChildPolicy::GetFieldValue(dstPath));
}

static void
_RemoveSpecFromLayer(
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath)
{
    switch (dstLayer->GetSpecType(dstPath)) {
    case SdfSpecTypeAttribute:
        _DoRemoveSpec<Sdf_AttributeChildPolicy>(dstLayer, dstPath);
        break;
    case SdfSpecTypeConnection:
        _DoRemoveSpec<Sdf_AttributeConnectionChildPolicy>(dstLayer, dstPath);
        break;
    case SdfSpecTypeExpression:
        _DoRemoveSpec<Sdf_ExpressionChildPolicy>(dstLayer, dstPath);
        break;
    case SdfSpecTypeMapper:
        _DoRemoveSpec<Sdf_MapperChildPolicy>(dstLayer, dstPath);
        break;
    case SdfSpecTypeMapperArg:
        _DoRemoveSpec<Sdf_MapperArgChildPolicy>(dstLayer, dstPath);
        break;
    case SdfSpecTypePrim:
        _DoRemoveSpec<Sdf_PrimChildPolicy>(dstLayer, dstPath);
        break;
    case SdfSpecTypeRelationship:
        _DoRemoveSpec<Sdf_RelationshipChildPolicy>(dstLayer, dstPath);
        break;
    case SdfSpecTypeRelationshipTarget:
        _DoRemoveSpec<Sdf_RelationshipTargetChildPolicy>(dstLayer, dstPath);
        break;
    case SdfSpecTypeVariant:
        _DoRemoveSpec<Sdf_VariantChildPolicy>(dstLayer, dstPath);
        break;
    case SdfSpecTypeVariantSet:
        _DoRemoveSpec<Sdf_VariantSetChildPolicy>(dstLayer, dstPath);
        break;

    case SdfSpecTypePseudoRoot:
    case SdfSpecTypeUnknown:
    case SdfNumSpecTypes:
        break;
    }
}

// Add a (field, value) entry to the list of fields to copy as directed by
// the given policy. The value may be empty to indicate that the field
// should be removed from the destination.
static void
_AddFieldValueToCopy(
    SdfSpecType specType, const TfToken& field,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool fieldInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
    const SdfShouldCopyValueFn& shouldCopyValue, _FieldValueList* valueList)
{
    std::optional<VtValue> value;
    if (shouldCopyValue(
            specType, field, 
            srcLayer, srcPath, fieldInSrc, dstLayer, dstPath, fieldInDst, 
            &value)) {

        // XXX: VtValue doesn't have move semantics...
        valueList->emplace_back(field, VtValue());
        if (value) {
            value->Swap(valueList->back().second);
        }
        else {
            srcLayer->GetField(srcPath, field).Swap(valueList->back().second);
        }
    }
}

// Call the given function for each field in srcFields and dstFields.
// The function will be called once for each unique field and will be 
// passed flags that indicate which container the field was in.
//
// The callable must have the signature: 
//    void (const TfToken& field, bool fieldInSrc, bool fieldInDst)
//
// srcFields and dstFields must be sorted using the TfTokenArbitraryLessThan
// comparator prior to calling this function.
//
template <class Callable>
static void
_ForEachField(
    const std::vector<TfToken>& srcFields,
    const std::vector<TfToken>& dstFields,
    const Callable& fn)
{
    auto lessThan = TfTokenFastArbitraryLessThan();

    auto srcIt = srcFields.begin(), srcEndIt = srcFields.end();
    auto dstIt = dstFields.begin(), dstEndIt = dstFields.end();
    while (srcIt != srcEndIt && dstIt != dstEndIt) {
        if (*srcIt == *dstIt) {
            fn(*srcIt, /* inSrc = */ true, /* inDst = */ true);
            ++srcIt, ++dstIt;
        }
        else if (lessThan(*srcIt, *dstIt)) {
            for (; srcIt != srcEndIt && lessThan(*srcIt, *dstIt); ++srcIt) {
                fn(*srcIt, /* inSrc = */ true, /* inDst = */ false);
            }
        }
        else {
            for (; dstIt != dstEndIt && lessThan(*dstIt, *srcIt); ++dstIt) {
                fn(*dstIt, /* inSrc = */ false, /* inDst = */ true);
            }
        }
    }

    auto finalIt = (srcIt == srcEndIt) ? dstIt : srcIt;
    auto finalEndIt = (srcIt == srcEndIt) ? dstEndIt : srcEndIt;
    const bool inSrc = (srcIt != srcEndIt);

    for (; finalIt != finalEndIt; ++finalIt) {
        fn(*finalIt, /* inSrc = */ inSrc, /* inDst = */ !inSrc);
    }
}

bool 
SdfCopySpec(
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath,
    const SdfShouldCopyValueFn& shouldCopyValueFn,
    const SdfShouldCopyChildrenFn& shouldCopyChildrenFn)
{
    if (!srcLayer || !dstLayer) {
        TF_CODING_ERROR("Invalid layer handle");
        return false;
    }

    if (srcPath.IsEmpty() || dstPath.IsEmpty()) {
        TF_CODING_ERROR("Invalid empty path");
        return false;
    }

    // Validate compatible source and destination path types.
    if ((srcPath.IsAbsoluteRootOrPrimPath()
                || srcPath.IsPrimVariantSelectionPath())
            != (dstPath.IsAbsoluteRootOrPrimPath()
                || dstPath.IsPrimVariantSelectionPath())
            || srcPath.IsPropertyPath() != dstPath.IsPropertyPath()
            || srcPath.IsTargetPath() != dstPath.IsTargetPath()
            || srcPath.IsMapperPath() != dstPath.IsMapperPath()
            || srcPath.IsMapperArgPath() != dstPath.IsMapperArgPath()
            || srcPath.IsExpressionPath() != dstPath.IsExpressionPath()) {
        TF_CODING_ERROR("Incompatible source and destination paths");
        return false;
    }

    // For target paths (relationship targets and connections), verify the
    // destination spec already exists.  See the documentation comment.
    if (dstPath.IsTargetPath() && !dstLayer->HasSpec(dstPath)) {
        TF_CODING_ERROR("Spec does not exist at destination target path");
        return false;
    }

    // If we're copying within a single layer and either srcPath or dstPath is a
    // prefix of the other, first copy the src scene description to a temporary
    // anonymous layer, then copy from _that_ temporary src to dstPath.  This
    // way we avoid mutating the src as we're copying.
    if (srcLayer == dstLayer && (srcPath.HasPrefix(dstPath) ||
                                 dstPath.HasPrefix(srcPath))) {
        // Create an anon layer with the same format & args as the src layer.
        SdfLayerRefPtr tmpSrcLayer =
            SdfLayer::CreateAnonymous("SdfCopySpec_tmp_src_layer",
                                      srcLayer->GetFileFormat(),
                                      srcLayer->GetFileFormatArguments());
        // Get the nearest prim path and copy that to the tmpSrc.  Note that
        // this in general copies more than necessary.  With more complicated
        // logic to create stub ancestors for all spec types we could narrow the
        // copying to just the src spec itself but we leave that to the future.
        SdfPath srcPrimPath = srcPath.GetPrimPath();
        SdfCreatePrimInLayer(tmpSrcLayer, srcPrimPath);

        if (!SdfCopySpec(srcLayer, srcPrimPath,
                         tmpSrcLayer, srcPrimPath)) {
            TF_RUNTIME_ERROR("Failed to create temporary source for overlapped "
                             "SdfCopySpec <%s> -> <%s>",
                             srcPath.GetAsString().c_str(),
                             dstPath.GetAsString().c_str());
            return false;
        }

        // Now copy the temporary src to the dst.
        return SdfCopySpec(tmpSrcLayer, srcPath,
                           dstLayer, dstPath,
                           shouldCopyValueFn, shouldCopyChildrenFn);
    }
    
    SdfChangeBlock block;

    // Create a stack of source/dest copy requests, initially populated with
    // the passed parameters.  The copy routine will add additional requests
    // as needed to handle children etc... and runs until the stack is empty.
    _CopyStack copyStack(1, _CopyStackEntry(srcPath, dstPath));
    while (!copyStack.empty()) {
        const _CopyStackEntry toCopy = copyStack.front();
        copyStack.pop_front();

        // If the source path is empty, it indicates that the spec at the
        // destination path should be removed.
        if (toCopy.srcPath.IsEmpty()) {
            _RemoveSpecFromLayer(dstLayer, toCopy.dstPath);
            continue;
        }

        // Figure out the concrete type of the spec we're copying. The spec type
        // dictates copying behavior below.
        const SdfSpecType specType = srcLayer->GetSpecType(toCopy.srcPath);
        if (specType == SdfSpecTypeUnknown) {
            TF_CODING_ERROR("Cannot copy unknown spec at <%s> from layer <%s>",
                toCopy.srcPath.GetText(), srcLayer->GetIdentifier().c_str());
            return false;
        }

        _SpecDataEntry copyEntry(toCopy.dstPath, specType);

        // Determine what data is present for the current source and dest specs
        // and what needs to be copied.
        const TfTokenVector
            dstValueFields = _GetValueFieldNames(dstLayer, toCopy.dstPath);
        const TfTokenVector
            srcValueFields = _GetValueFieldNames(srcLayer, toCopy.srcPath);

        // From the list of value fields, retrieve all values that the copy
        // policy says we need to copy over to the destination.
        _ForEachField(
            srcValueFields, dstValueFields,
            [&](const TfToken& field, bool fieldInSrc, bool fieldInDst) {
                _AddFieldValueToCopy(
                    specType, field, 
                    srcLayer, toCopy.srcPath, fieldInSrc,
                    dstLayer, toCopy.dstPath, fieldInDst,
                    shouldCopyValueFn, &copyEntry.dataToCopy);
            });

        // Since prims and variants hold the same information, a prim can be
        // copied to a variant and vice-versa. If this is the case, we need
        // to update the copy entry since the code below expects the source
        // and destination spec types to be the same.
        const bool copyingPrimToVariant = 
            specType == SdfSpecTypePrim && 
            toCopy.dstPath.IsPrimVariantSelectionPath();
        const bool copyingVariantToPrim =
            specType == SdfSpecTypeVariant && toCopy.dstPath.IsPrimPath();

        if (copyingPrimToVariant || copyingVariantToPrim) {
            // Clear out any specifier or typename fields in the data to copy,
            // since we'll want to set those specially.
            copyEntry.dataToCopy.erase(
                std::remove_if(
                    copyEntry.dataToCopy.begin(), copyEntry.dataToCopy.end(),
                    [](const _FieldValuePair& fv) {
                        return fv.first == SdfFieldKeys->Specifier ||
                            fv.first == SdfFieldKeys->TypeName;
                    }),
                copyEntry.dataToCopy.end());

            if (copyingPrimToVariant) {
                // Set the specifier for the destination variant to over, since
                // that's the value used in SdfVariantSpec's c'tor.
                copyEntry.dataToCopy.push_back({
                    SdfFieldKeys->Specifier, VtValue(SdfSpecifierOver)});
                copyEntry.specType = SdfSpecTypeVariant;
            }
            else if (copyingVariantToPrim) {
                // Variants don't have a specifier or typename, but for
                // convenience we copy those values from the owning prim.
                const SdfPath srcPrimPath = toCopy.srcPath.GetPrimPath();
                std::vector<TfToken> srcFields, dstFields;
                for (const TfToken& field : 
                    { SdfFieldKeys->Specifier, SdfFieldKeys->TypeName } ) {

                    if (srcLayer->HasField(srcPrimPath, field)) {
                        srcFields.push_back(field);
                    }
                    if (dstLayer->HasField(toCopy.dstPath, field)) {
                        dstFields.push_back(field);
                    }
                }

                _ForEachField(
                    srcFields, dstFields,
                    [&](const TfToken& field, bool fieldInSrc, bool fieldInDst) {
                        _AddFieldValueToCopy(
                            specType, field, 
                            srcLayer, srcPrimPath, fieldInSrc,
                            dstLayer, toCopy.dstPath, fieldInDst,
                            shouldCopyValueFn, &copyEntry.dataToCopy);
                    });

                copyEntry.specType = SdfSpecTypePrim;
            }
        }

        // Create the new spec and copy all of the specified fields over.
        _AddNewSpecToLayer(dstLayer, copyEntry);
        for (const _FieldValuePair& fieldValue : copyEntry.dataToCopy) {
            if (fieldValue.second.IsHolding<SdfCopySpecsValueEdit>()) {
                const SdfCopySpecsValueEdit::EditFunction& edit = 
                    fieldValue.second.UncheckedGet<SdfCopySpecsValueEdit>()
                    .GetEditFunction();
                edit(dstLayer, copyEntry.dstPath);
            }
            else {
                dstLayer->SetField(
                    copyEntry.dstPath, fieldValue.first, fieldValue.second);
            }
        }

        // Retrieve the children fields to be copied. Don't retrieve them before
        // value fields are copied as children fields may be modified during
        // copying value fields.  This is mainly due to abstract data
        // implementations that "procedurally" present children fields based on
        // other fields.
        const TfTokenVector dstChildrenFields =
            _GetChildrenFieldNames(dstLayer, toCopy.dstPath);
        const TfTokenVector srcChildrenFields =
            _GetChildrenFieldNames(srcLayer, toCopy.srcPath);

        // Now add any children specs that need to be copied to our
        // copy stack.
        _ForEachField(
            srcChildrenFields, dstChildrenFields,
            [&](const TfToken& field, bool fieldInSrc, bool fieldInDst) {
                _ProcessChildField(
                    field,
                    srcLayer, toCopy.srcPath, fieldInSrc,
                    dstLayer, toCopy.dstPath, fieldInDst,
                    shouldCopyChildrenFn, &copyStack);
            });
    }
    
    return true;
}

// ------------------------------------------------------------

template <class T>
static T 
_FixInternalSubrootPaths(
    const T& ref, const SdfPath& srcPrefix, const SdfPath& dstPrefix)
{
    // Only try to fix up internal sub-root references.
    if (!ref.GetAssetPath().empty() ||
        ref.GetPrimPath().IsEmpty() ||
        ref.GetPrimPath().IsRootPrimPath()) {
        return ref;
    }

    T fixedRef = ref;
    fixedRef.SetPrimPath(ref.GetPrimPath().ReplacePrefix(srcPrefix, dstPrefix));

    return fixedRef;
}

bool
SdfShouldCopyValue(
    const SdfPath& srcRootPath, const SdfPath& dstRootPath,
    SdfSpecType specType, const TfToken& field,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool fieldInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
    std::optional<VtValue>* valueToCopy)
{
    if (fieldInSrc) {
        if (field == SdfFieldKeys->ConnectionPaths || 
            field == SdfFieldKeys->TargetPaths ||
            field == SdfFieldKeys->InheritPaths ||
            field == SdfFieldKeys->Specializes) {
            SdfPathListOp srcListOp;
            if (srcLayer->HasField(srcPath, field, &srcListOp)) {
                const SdfPath& srcPrefix = 
                    srcRootPath.GetPrimPath().StripAllVariantSelections();
                const SdfPath& dstPrefix = 
                    dstRootPath.GetPrimPath().StripAllVariantSelections();

                srcListOp.ModifyOperations(
                    [&srcPrefix, &dstPrefix](const SdfPath& path) {
                        return path.ReplacePrefix(srcPrefix, dstPrefix);
                    });

                *valueToCopy = VtValue::Take(srcListOp);
            }
        }
        else if (field == SdfFieldKeys->References) {
            SdfReferenceListOp refListOp;
            if (srcLayer->HasField(srcPath, field, &refListOp)) {
                const SdfPath& srcPrefix = 
                    srcRootPath.GetPrimPath().StripAllVariantSelections();
                const SdfPath& dstPrefix = 
                    dstRootPath.GetPrimPath().StripAllVariantSelections();

                refListOp.ModifyOperations(
                    std::bind(&_FixInternalSubrootPaths<SdfReference>, 
                              std::placeholders::_1, std::cref(srcPrefix), 
                              std::cref(dstPrefix)));

                *valueToCopy = VtValue::Take(refListOp);
            }
        }
        else if (field == SdfFieldKeys->Payload) {
            SdfPayloadListOp payloadListOp;
            if (srcLayer->HasField(srcPath, field, &payloadListOp)) {
                const SdfPath& srcPrefix =
                    srcRootPath.GetPrimPath().StripAllVariantSelections();
                const SdfPath& dstPrefix =
                    dstRootPath.GetPrimPath().StripAllVariantSelections();

                payloadListOp.ModifyOperations(
                    std::bind(&_FixInternalSubrootPaths<SdfPayload>, 
                              std::placeholders::_1, std::cref(srcPrefix), 
                              std::cref(dstPrefix)));

                *valueToCopy = VtValue::Take(payloadListOp);
            }
        }
        else if (field == SdfFieldKeys->Relocates) {
            SdfRelocatesMap relocates;
            if (srcLayer->HasField(srcPath, field, &relocates)) {
                const SdfPath& srcPrefix = 
                    srcRootPath.GetPrimPath().StripAllVariantSelections();
                const SdfPath& dstPrefix = 
                    dstRootPath.GetPrimPath().StripAllVariantSelections();

                SdfRelocatesMap updatedRelocates;
                for (const auto& entry : relocates) {
                    const SdfPath updatedSrcPath = 
                        entry.first.ReplacePrefix(srcPrefix, dstPrefix);
                    const SdfPath updatedTargetPath = 
                        entry.second.ReplacePrefix(srcPrefix, dstPrefix);
                    updatedRelocates[updatedSrcPath] = updatedTargetPath;
                }

                *valueToCopy = VtValue::Take(updatedRelocates);
            }
        }
    }

    return true;
}

bool
SdfShouldCopyChildren(
    const SdfPath& srcRootPath, const SdfPath& dstRootPath,
    const TfToken& childrenField,
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath, bool fieldInSrc,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath, bool fieldInDst,
    std::optional<VtValue>* srcChildren,
    std::optional<VtValue>* dstChildren)
{
    if (fieldInSrc) {
        if (childrenField == SdfChildrenKeys->ConnectionChildren ||
            childrenField == SdfChildrenKeys->RelationshipTargetChildren ||
            childrenField == SdfChildrenKeys->MapperChildren) {

            SdfPathVector children;
            if (srcLayer->HasField(srcPath, childrenField, &children)) {
                *srcChildren = VtValue(children);

                const SdfPath& srcPrefix = 
                    srcRootPath.GetPrimPath().StripAllVariantSelections();
                const SdfPath& dstPrefix = 
                    dstRootPath.GetPrimPath().StripAllVariantSelections();

                for (SdfPath& child : children) {
                    child = child.ReplacePrefix(srcPrefix, dstPrefix);
                }

                *dstChildren = VtValue::Take(children);
            }
        }
    }

    return true;
}

bool
SdfCopySpec(
    const SdfLayerHandle& srcLayer, const SdfPath& srcPath,
    const SdfLayerHandle& dstLayer, const SdfPath& dstPath)
{
    namespace ph = std::placeholders;

    return SdfCopySpec(
        srcLayer, srcPath, dstLayer, dstPath,
        /* shouldCopyValueFn = */ std::bind(
            SdfShouldCopyValue,
            std::cref(srcPath), std::cref(dstPath),
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6, ph::_7, ph::_8, 
            ph::_9),
        /* shouldCopyChildrenFn = */ std::bind(
            SdfShouldCopyChildren,
            std::cref(srcPath), std::cref(dstPath),
            ph::_1, ph::_2, ph::_3, ph::_4, ph::_5, ph::_6, ph::_7, ph::_8, 
            ph::_9)
        );
}

}  // namespace pxr
