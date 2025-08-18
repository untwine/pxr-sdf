//
// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
#include <pxr/sdf/pxr.h>
#include <pxr/sdf/timeCode.h>
#include <pxr/sdf/layerOffset.h>

#include <pxr/tf/registryManager.h>
#include <pxr/vt/array.h>
#include <pxr/vt/arrayEdit.h>
#include <pxr/vt/value.h>
#include <pxr/vt/valueTransform.h>

SDF_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(VtValue)
{
    VtRegisterTransform(
        +[](GfTimeCode const &timeCode, SdfLayerOffset const &offset) {
            return offset * timeCode;
        });
}

SDF_NAMESPACE_CLOSE_SCOPE
