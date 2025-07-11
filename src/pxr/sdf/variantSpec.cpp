// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

/// \file VariantSpec.cpp

#include "./variantSpec.h"
#include "./changeBlock.h"
#include "./childrenUtils.h"
#include "./layer.h"
#include "./primSpec.h"
#include "./schema.h"
#include "./variantSetSpec.h"
#include <pxr/tf/diagnostic.h>
#include <pxr/tf/type.h>
#include <pxr/trace/trace.h>

using std::string;

namespace pxr {

SDF_DEFINE_SPEC(SdfSchema, SdfSpecTypeVariant, SdfVariantSpec, SdfSpec);

SdfVariantSpecHandle
SdfVariantSpec::New(
    const SdfVariantSetSpecHandle& owner, const std::string& name)
{
    TRACE_FUNCTION();

    if (!owner) {
        TF_CODING_ERROR("NULL owner variant set");
        return TfNullPtr;
    }

    if (!SdfSchema::IsValidVariantIdentifier(name)) {
        TF_CODING_ERROR("Invalid variant name: %s", name.c_str());
        return TfNullPtr;
    }

    SdfPath childPath = 
        Sdf_VariantChildPolicy::GetChildPath(owner->GetPath(), TfToken(name));

    SdfLayerHandle layer = owner->GetLayer();
    if (!Sdf_ChildrenUtils<Sdf_VariantChildPolicy>::CreateSpec(
            layer, childPath, SdfSpecTypeVariant)) {
        return TfNullPtr;
    }

    layer->SetField(childPath, SdfFieldKeys->Specifier, SdfSpecifierOver);

    return TfStatic_cast<SdfVariantSpecHandle>(
        layer->GetObjectAtPath(childPath));
}


//
// Name
//

std::string
SdfVariantSpec::GetName() const
{
    return GetPath().GetVariantSelection().second;
}

TfToken
SdfVariantSpec::GetNameToken() const
{
    return TfToken(GetPath().GetVariantSelection().second);
}

//
// Namespace hierarchy
//

SdfVariantSetSpecHandle
SdfVariantSpec::GetOwner() const
{
    SdfPath path = Sdf_VariantChildPolicy::GetParentPath(GetPath());
    return TfStatic_cast<SdfVariantSetSpecHandle>(
        GetLayer()->GetObjectAtPath(path));
}

SdfPrimSpecHandle
SdfVariantSpec::GetPrimSpec() const
{
    return GetLayer()->GetPrimAtPath(GetPath());
}

SdfVariantSetsProxy
SdfVariantSpec::GetVariantSets() const
{
    return SdfVariantSetsProxy(SdfVariantSetView(GetLayer(),
            GetPath(), SdfChildrenKeys->VariantSetChildren),
            "variant sets", SdfVariantSetsProxy::CanErase);
}

std::vector<std::string> 
SdfVariantSpec::GetVariantNames(const std::string& name) const
{
    std::vector<std::string> variantNames;

    SdfPath variantSetPath = GetPath().AppendVariantSelection(name, "");
    std::vector<TfToken> variantNameTokens =
        GetLayer()->GetFieldAs<std::vector<TfToken> >(variantSetPath,
            SdfChildrenKeys->VariantChildren);

    variantNames.reserve(variantNameTokens.size());
    TF_FOR_ALL(i, variantNameTokens) {
        variantNames.push_back(i->GetString());
    }

    return variantNames;
}

SdfVariantSpecHandle
SdfCreateVariantInLayer(
    const SdfLayerHandle &layer,
    const SdfPath &primPath,
    const string &variantSetName,
    const string &variantName )
{
    SdfPath variantPath =
        primPath.AppendVariantSelection(variantSetName, variantName);

    // Create the ancestor prim and variant specs.
    SdfCreatePrimInLayer(layer, variantPath);

    // Look up the resulting variant spec.
    return TfStatic_cast<SdfVariantSpecHandle>(
        layer->GetObjectAtPath(variantPath));
}

}  // namespace pxr
