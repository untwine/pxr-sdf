// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/pxr.h>
#include <pxr/sdf/path.h>
#include <pxr/vt/array.h>
#include <pxr/vt/wrapArray.h>
#include <pxr/vt/valueFromPython.h>

SDF_NAMESPACE_OPEN_SCOPE

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

SDF_NAMESPACE_CLOSE_SCOPE

SDF_NAMESPACE_USING_DIRECTIVE

void wrapArrayPath() {
    VtWrapArray<VtArray<SdfPath> >();
    VtValueFromPythonLValue<VtArray<SdfPath> >();
}
