// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./opaqueValue.h"

#include <pxr/tf/registryManager.h>
#include <pxr/tf/type.h>
#include <pxr/vt/array.h>

#include <iostream>


namespace pxr {

TF_REGISTRY_FUNCTION(pxr::TfType)
{
    TfType::Define<SdfOpaqueValue>();
    // Even though we don't support an opaque[] type in scene description, there
    // is still code that assumes that any scene-description value type has a
    // TfType-registered array type too, so we register it here as well.
    TfType::Define<VtArray<SdfOpaqueValue>>();
}

std::ostream &
operator<<(std::ostream &s, SdfOpaqueValue const &)
{
    return s << "OpaqueValue";
}

}  // namespace pxr
