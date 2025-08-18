// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "pxr/sdf/pxr.h"
#include <pxr/arch/virtualMemory.h>
#include "pxr/sdf/pool.h"

SDF_NAMESPACE_OPEN_SCOPE

char *
Sdf_PoolReserveRegion(size_t numBytes)
{
    return static_cast<char *>(ArchReserveVirtualMemory(numBytes));
}

bool
Sdf_PoolCommitRange(char *start, char *end)
{
    return ArchCommitVirtualMemoryRange(start, end-start);
}

SDF_NAMESPACE_CLOSE_SCOPE
