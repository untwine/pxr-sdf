// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

/// \file PseudoRootSpec.cpp

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/pseudoRootSpec.h"
#include <pxr/tf/type.h>

SDF_NAMESPACE_OPEN_SCOPE

SDF_DEFINE_SPEC(
    SdfSchema, SdfSpecTypePseudoRoot, SdfPseudoRootSpec, SdfPrimSpec);

SDF_NAMESPACE_CLOSE_SCOPE
