// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/assetPath.h>
#include <pxr/vt/array.h>
#include <pxr/vt/wrapArray.h>
#include <pxr/vt/valueFromPython.h>

namespace pxr {

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

}  // namespace pxr

using namespace pxr;

void wrapArrayAssetPath() {
    VtWrapArray<VtArray<SdfAssetPath> >();
    VtValueFromPythonLValue<VtArray<SdfAssetPath> >();
}
