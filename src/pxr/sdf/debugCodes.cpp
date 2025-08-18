// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/debugCodes.h"
#include <pxr/tf/debug.h>
#include <pxr/tf/registryManager.h>

SDF_NAMESPACE_OPEN_SCOPE

TF_REGISTRY_FUNCTION(TfDebug)
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

SDF_NAMESPACE_CLOSE_SCOPE
