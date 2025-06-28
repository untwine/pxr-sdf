// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./debugCodes.h"
#include <pxr/tf/debug.h>
#include <pxr/tf/registryManager.h>

namespace pxr {

TF_REGISTRY_FUNCTION(pxr::TfDebug)
{
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        SDF_ASSET, "Sdf asset resolution diagnostics");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        SDF_CHANGES, "Sdf layer change notifications");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        SDF_FILE_FORMAT, "Sdf file format registration");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        SDF_LAYER, "Sdf layer loading and lifetime");
    TF_DEBUG_ENVIRONMENT_SYMBOL(
        SDF_VARIABLE_EXPRESSION_PARSING, 
        "Sdf variable expression parsing");
}

}  // namespace pxr
