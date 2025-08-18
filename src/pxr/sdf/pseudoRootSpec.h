// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_PSEUDO_ROOT_SPEC_H
#define PXR_SDF_PSEUDO_ROOT_SPEC_H

/// \file sdf/pseudoRootSpec.h

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/declareSpec.h"
#include "pxr/sdf/primSpec.h"

SDF_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfPseudoRootSpec);

class SdfPseudoRootSpec : public SdfPrimSpec
{
    SDF_DECLARE_SPEC(SdfPseudoRootSpec, SdfPrimSpec);
};

SDF_NAMESPACE_CLOSE_SCOPE

#endif // PXR_SDF_PSEUDO_ROOT_SPEC_H
