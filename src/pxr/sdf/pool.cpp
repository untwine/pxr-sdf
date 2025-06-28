// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/arch/virtualMemory.h>
#include "./pool.h"

namespace pxr {

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

}  // namespace pxr
