// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/pathTable.h"
#include <pxr/tf/pyLock.h>
#include <pxr/work/loops.h>

SDF_NAMESPACE_OPEN_SCOPE

void
Sdf_VisitPathTableInParallel(void **entryStart, size_t numEntries,
                             TfFunctionRef<void(void*&)> const visitFn)
{
    // We must release the GIL here if we have it; otherwise, if visitFn
    // attempted to take the GIL, the workers would deadlock.
    TF_PY_ALLOW_THREADS_IN_SCOPE();

    WorkParallelForN(
        numEntries,
        [&entryStart, visitFn](size_t i, size_t end) {
            for (; i != end; ++i) {
                if (entryStart[i]) {
                    visitFn(entryStart[i]);
                }
            }
        });
}

SDF_NAMESPACE_CLOSE_SCOPE
