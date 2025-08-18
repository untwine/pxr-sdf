// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/cleanupEnabler.h"
#include "pxr/sdf/cleanupTracker.h"
#include <pxr/tf/instantiateStacked.h>

SDF_NAMESPACE_OPEN_SCOPE

TF_INSTANTIATE_DEFINED_STACKED(SdfCleanupEnabler);

SdfCleanupEnabler::SdfCleanupEnabler()
{
    // Do nothing
}

SdfCleanupEnabler::~SdfCleanupEnabler()
{
    if (GetStack().size() == 1) {
        // The last CleanupEnabler is being removed from the stack, so notify 
        // the CleanupTracker that it's time to clean up any specs it collected.
        Sdf_CleanupTracker::GetInstance().CleanupSpecs();
    }
}

// Static
bool
SdfCleanupEnabler::IsCleanupEnabled()
{
    return !GetStack().empty();
}

SDF_NAMESPACE_CLOSE_SCOPE
