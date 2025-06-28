// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/path.h>
#include <pxr/vt/array.h>
#include <pxr/vt/wrapArray.h>
#include <pxr/vt/valueFromPython.h>

namespace pxr {

namespace Vt_WrapArray {
    template <>
    std::string GetVtArrayName< VtArray<SdfPath> >() {
        return "PathArray";
    }
}

template<>
SdfPath VtZero() {
    return SdfPath();
}

}  // namespace pxr

using namespace pxr;

void wrapArrayPath() {
    VtWrapArray<VtArray<SdfPath> >();
    VtValueFromPythonLValue<VtArray<SdfPath> >();
}
