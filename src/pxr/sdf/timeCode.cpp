// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./timeCode.h"

#include <pxr/tf/registryManager.h>
#include <pxr/tf/type.h>

#include <pxr/vt/array.h>
#include <pxr/vt/value.h>

#include <ostream>

namespace pxr {

// Register this class with the TfType registry
// Array registration included to facilitate Sdf/Types and Sdf/ParserHelpers
TF_REGISTRY_FUNCTION(pxr::TfType)
{
    TfType::Define<SdfTimeCode>();
    TfType::Define< VtArray<SdfTimeCode> >();
}

TF_REGISTRY_FUNCTION(pxr::VtValue)
{
    VtValue::RegisterSimpleBidirectionalCast<double, SdfTimeCode>();
}

std::ostream& 
operator<<(std::ostream& out, const SdfTimeCode& ap)
{
    return out << ap.GetValue();
}

}  // namespace pxr
