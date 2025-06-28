// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/timeCode.h>
#include <pxr/vt/array.h>
#include <pxr/vt/wrapArray.h>
#include <pxr/vt/valueFromPython.h>

namespace pxr {

namespace Vt_WrapArray {
    template <>
    std::string GetVtArrayName< VtArray<SdfTimeCode> >() {
        return "TimeCodeArray";
    }
}

}  // namespace pxr

using namespace pxr;

void wrapArrayTimeCode() {
    VtWrapArray<VtArray<SdfTimeCode> >();
    VtValueFromPythonLValue<VtArray<SdfTimeCode> >();
}
