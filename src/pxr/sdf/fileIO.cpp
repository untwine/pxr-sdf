// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

///
/// \file sdf/fileIO.cpp

#include "./fileIO.h"

#include "./attributeSpec.h"
#include "./fileIO_Common.h"
#include "./primSpec.h"
#include "./relationshipSpec.h"
#include "./variantSetSpec.h"
#include "./variantSpec.h"

#include <pxr/tf/stringUtils.h>

#include <ostream>

namespace pxr {

Sdf_StreamWritableAsset::~Sdf_StreamWritableAsset() = default;

bool
Sdf_WriteToStream(const SdfSpec &baseSpec, std::ostream& o, size_t indent)
{
    Sdf_TextOutput out(o);

    const SdfSpecType type = baseSpec.GetSpecType();

    switch (type) {
    case SdfSpecTypePrim:
        {
            SdfPrimSpec spec =
                Sdf_CastAccess::CastSpec<SdfPrimSpec, SdfSpec>(baseSpec);
            return Sdf_WritePrim(spec, out, indent);
        }
    case SdfSpecTypeAttribute:
        {
            SdfAttributeSpec spec =
                Sdf_CastAccess::CastSpec<SdfAttributeSpec, SdfSpec>(baseSpec);
            return Sdf_WriteAttribute(spec, out, indent);
        }
    case SdfSpecTypeRelationship:
        {
            SdfRelationshipSpec spec =
                Sdf_CastAccess::CastSpec<SdfRelationshipSpec, SdfSpec>(baseSpec);
            return Sdf_WriteRelationship(spec, out, indent);
        }
    case SdfSpecTypeVariantSet:
        {
            SdfVariantSetSpec spec =
                Sdf_CastAccess::CastSpec<SdfVariantSetSpec, SdfSpec>(baseSpec);
            return Sdf_WriteVariantSet(spec, out, indent);
        }
    case SdfSpecTypeVariant:
        {
            SdfVariantSpec spec =
                Sdf_CastAccess::CastSpec<SdfVariantSpec, SdfSpec>(baseSpec);
            return Sdf_WriteVariant(spec, out, indent);
        }
    default:
        break;
    }

    TF_CODING_ERROR("Cannot write spec of type %s to stream", 
                    TfStringify(type).c_str());
    return false;
}

}  // namespace pxr
