// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

/// \file RelationshipSpec.cpp

#include "./relationshipSpec.h"
#include "./accessorHelpers.h"
#include "./attributeSpec.h"
#include "./changeBlock.h"
#include "./childrenUtils.h"
#include "./childrenPolicies.h"
#include "./layer.h"
#include "./primSpec.h"
#include "./proxyTypes.h"
#include "./schema.h"

#include <pxr/tf/type.h>
#include <pxr/trace/trace.h>

#include <functional>
#include <optional>

namespace pxr {

SDF_DEFINE_SPEC(
    SdfSchema, SdfSpecTypeRelationship, SdfRelationshipSpec, SdfPropertySpec);

//
// Primary API
//

SdfRelationshipSpecHandle
SdfRelationshipSpec::New(
    const SdfPrimSpecHandle& owner,
    const std::string& name,
    bool custom,
    SdfVariability variability)
{
    TRACE_FUNCTION();

    if (!owner) {
        TF_CODING_ERROR("NULL owner prim");
        return TfNullPtr;
    }

    if (!Sdf_ChildrenUtils<Sdf_RelationshipChildPolicy>::IsValidName(name)) {
        TF_CODING_ERROR("Cannot create a relationship on %s with "
            "invalid name: %s", owner->GetPath().GetText(), name.c_str());
        return TfNullPtr;
    }

    SdfPath relPath = owner->GetPath().AppendProperty(TfToken(name));
    if (!relPath.IsPropertyPath()) {
        TF_CODING_ERROR(
            "Cannot create relationship at invalid path <%s.%s>",
            owner->GetPath().GetText(), name.c_str());
        return TfNullPtr;
    }
    
    // RelationshipSpecs are considered initially to have only required fields 
    // only if they are not custom.
    bool hasOnlyRequiredFields = (!custom);

    SdfChangeBlock block;

    if (!Sdf_ChildrenUtils<Sdf_RelationshipChildPolicy>::CreateSpec(
            owner->GetLayer(), relPath, SdfSpecTypeRelationship, 
            hasOnlyRequiredFields)) {
        return TfNullPtr;
    }

    SdfRelationshipSpecHandle spec =
        owner->GetLayer()->GetRelationshipAtPath(relPath);

    spec->SetField(SdfFieldKeys->Custom, custom);
    spec->SetField(SdfFieldKeys->Variability, variability);

    return spec;
}

//
// Relationship Targets
//

SdfPath 
SdfRelationshipSpec::_CanonicalizeTargetPath(const SdfPath& path) const
{
    // Relationship target paths are always absolute. If a relative path
    // is passed in, it is considered to be relative to the relationship's
    // owning prim.
    return path.MakeAbsolutePath(GetPath().GetPrimPath());
}

SdfPath 
SdfRelationshipSpec::_MakeCompleteTargetSpecPath(
    const SdfPath& targetPath) const
{
    SdfPath absPath = _CanonicalizeTargetPath(targetPath);
    return GetPath().AppendTarget(absPath);
}

SdfSpecHandle 
SdfRelationshipSpec::_GetTargetSpec(const SdfPath& path) const
{
    return GetLayer()->GetObjectAtPath(
        _MakeCompleteTargetSpecPath(path));
}

SdfTargetsProxy
SdfRelationshipSpec::GetTargetPathList() const
{
    return SdfGetPathEditorProxy(
        SdfCreateHandle(this), SdfFieldKeys->TargetPaths);
}

bool
SdfRelationshipSpec::HasTargetPathList() const
{
    return GetTargetPathList().HasKeys();
}

void
SdfRelationshipSpec::ClearTargetPathList() const
{
    GetTargetPathList().ClearEdits();
}

static std::optional<SdfPath>
_ReplacePath(
    const SdfPath &oldPath, const SdfPath &newPath, const SdfPath &path)
{
    // Replace oldPath with newPath, and also remove any existing
    // newPath entries in the list op.
    if (path == oldPath) {
        return newPath;
    }
    if (path == newPath) {
        return std::nullopt;
    }
    return path;
}

void
SdfRelationshipSpec::ReplaceTargetPath(
    const SdfPath& oldPath,
    const SdfPath& newPath)
{
    // Check permissions; this is done here to catch the case where ChangePaths
    // is not called due to an erroneous oldPath being supplied, and ModifyEdits
    // won't check either if there are no changes made.
    if (!PermissionToEdit()) {
        TF_CODING_ERROR("ReplaceTargetPath: Permission denied.");
        return;
    }

    const SdfPath& relPath = GetPath();
    const SdfLayerHandle& layer = GetLayer();

    SdfPath oldTargetPath = _CanonicalizeTargetPath(oldPath);
    SdfPath newTargetPath = _CanonicalizeTargetPath(newPath);
    
    if (oldTargetPath == newTargetPath) {
        return;
    }

    // Get the paths of all the existing target specs
    std::vector<SdfPath> siblingPaths = 
	layer->GetFieldAs<std::vector<SdfPath> >(
            relPath, SdfChildrenKeys->RelationshipTargetChildren);

    int oldTargetSpecIndex = -1;
    int newTargetSpecIndex = -1;
    for (size_t i = 0, n = siblingPaths.size(); i != n; ++i) {
        if (siblingPaths[i] == oldTargetPath) {
            oldTargetSpecIndex = i;
        }
        else if (siblingPaths[i] == newTargetPath) {
            newTargetSpecIndex = i;
        }
    }
    
    // If there is a target spec, then update the children field.
    if (oldTargetSpecIndex != -1) {
        SdfPath oldTargetSpecPath = relPath.AppendTarget(oldTargetPath);
        SdfPath newTargetSpecPath = relPath.AppendTarget(newTargetPath);

        if (layer->HasSpec(newTargetSpecPath)) {
            // Target already exists.  If the target has no child specs
            // then we'll allow the replacement.  If it does have
            // attributes then we must refuse.
            const SdfSchemaBase& schema = GetSchema();
            for (const TfToken& field : layer->ListFields(newTargetSpecPath)) {
                if (schema.HoldsChildren(field)) {
                    TF_CODING_ERROR("Can't replace target %s with target %s in "
                                    "relationship %s: %s",
                                    oldPath.GetText(),
                                    newPath.GetText(),
                                    relPath.GetString().c_str(),
                                    "Target already exists");
                    return;
                }
            }

            // Remove the existing spec at the new target path.
            _DeleteSpec(newTargetSpecPath);

            TF_VERIFY (!layer->HasSpec(newTargetSpecPath));
        }
        
        // Move the spec and all the fields under it.
        if (!_MoveSpec(oldTargetSpecPath, newTargetSpecPath)) {
            TF_CODING_ERROR("Cannot move %s to %s", oldTargetPath.GetText(),
                newTargetPath.GetText());
            return;
        }

        // Update and set the siblings
        siblingPaths[oldTargetSpecIndex] = newTargetPath;
        if (newTargetSpecIndex != -1) {
            siblingPaths.erase(siblingPaths.begin() + newTargetSpecIndex);
        }
        
        layer->SetField(relPath, SdfChildrenKeys->RelationshipTargetChildren,
            siblingPaths);
    }

    // Get the list op.
    SdfPathListOp targetsListOp = 
        layer->GetFieldAs<SdfPathListOp>(relPath, SdfFieldKeys->TargetPaths);

    // Update the list op.
    if (targetsListOp.HasItem(oldTargetPath)) {
        targetsListOp.ModifyOperations(
            std::bind(_ReplacePath, oldTargetPath, newTargetPath,
                std::placeholders::_1));
        layer->SetField(relPath, SdfFieldKeys->TargetPaths, targetsListOp);
    }
}

void
SdfRelationshipSpec::RemoveTargetPath(
    const SdfPath& path,
    bool preserveTargetOrder)
{
    const SdfPath targetSpecPath = 
        GetPath().AppendTarget(_CanonicalizeTargetPath(path));

    SdfChangeBlock block;
    Sdf_ChildrenUtils<Sdf_AttributeChildPolicy>::SetChildren(
        GetLayer(), targetSpecPath, 
        std::vector<SdfAttributeSpecHandle>());

    // The SdfTargetsProxy will manage conversion of the SdfPaths and changes to
    // both the list edits and actual object hierarchy underneath.
    if (preserveTargetOrder) {
        GetTargetPathList().Erase(path);
    }
    else {
        GetTargetPathList().RemoveItemEdits(path);
    }
}

//
// Metadata, Property Value API, and Spec Properties
// (methods built on generic SdfSpec accessor macros)
//

// Initialize accessor helper macros to associate with this class and optimize
// out the access predicate
#define SDF_ACCESSOR_CLASS                   SdfRelationshipSpec
#define SDF_ACCESSOR_READ_PREDICATE(key_)    SDF_NO_PREDICATE
#define SDF_ACCESSOR_WRITE_PREDICATE(key_)   SDF_NO_PREDICATE

SDF_DEFINE_GET_SET(NoLoadHint, SdfFieldKeys->NoLoadHint, bool);

#undef SDF_ACCESSOR_CLASS                   
#undef SDF_ACCESSOR_READ_PREDICATE
#undef SDF_ACCESSOR_WRITE_PREDICATE

// Defined in primSpec.cpp.
bool
Sdf_UncheckedCreatePrimInLayer(SdfLayer *layer, SdfPath const &primPath);

SdfRelationshipSpecHandle
SdfCreateRelationshipInLayer(
    const SdfLayerHandle &layer,
    const SdfPath &relPath,
    SdfVariability variability,
    bool isCustom)
{
    if (SdfJustCreateRelationshipInLayer(layer, relPath,
                                         variability, isCustom)) {
        return layer->GetRelationshipAtPath(relPath);
    }
    return TfNullPtr;
}

bool
SdfJustCreateRelationshipInLayer(
    const SdfLayerHandle &layer,
    const SdfPath &relPath,
    SdfVariability variability,
    bool isCustom)
{
    if (!relPath.IsPrimPropertyPath()) {
        TF_CODING_ERROR("Cannot create prim relationship at path '%s' because "
                        "it is not a prim property path",
                        relPath.GetText());
        return false;
    }

    SdfLayer *layerPtr = get_pointer(layer);

    SdfChangeBlock block;

    if (!Sdf_UncheckedCreatePrimInLayer(layerPtr, relPath.GetParentPath())) {
        return false;
    }

    if (!Sdf_ChildrenUtils<Sdf_RelationshipChildPolicy>::CreateSpec(
            layer, relPath, SdfSpecTypeRelationship,
            /*hasOnlyRequiredFields=*/!isCustom)) {
        TF_RUNTIME_ERROR("Failed to create relationship at path '%s' in "
                         "layer @%s@", relPath.GetText(),
                         layerPtr->GetIdentifier().c_str());
        return false;
    }
    
    layerPtr->SetField(relPath, SdfFieldKeys->Custom, isCustom);
    layerPtr->SetField(relPath, SdfFieldKeys->Variability, variability);
    
    return true;
}

}  // namespace pxr
