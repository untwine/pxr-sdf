// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/pxr.h>
#include <pxr/sdf/assetPath.h>
#include <pxr/vt/array.h>
#include <pxr/vt/wrapArray.h>
#include <pxr/vt/valueFromPython.h>

SDF_NAMESPACE_OPEN_SCOPE

namespace Vt_WrapArray {
    template <>
    std::string GetVtArrayName< VtArray<SdfAssetPath> >() {
        return "AssetPathArray";
    }
}

template<>
SdfAssetPath VtZero() {
    return SdfAssetPath();
}

SDF_NAMESPACE_CLOSE_SCOPE

SDF_NAMESPACE_USING_DIRECTIVE

void wrapArrayAssetPath() {
    VtWrapArray<VtArray<SdfAssetPath> >();
    VtValueFromPythonLValue<VtArray<SdfAssetPath> >();
}
