// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_PY_UTILS_H
#define PXR_SDF_PY_UTILS_H

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/api.h"
#include "pxr/sdf/layer.h"

#include <pxr/boost/python/dict.hpp>
#include <string>

SDF_NAMESPACE_OPEN_SCOPE

/// Convert the Python dictionary \p dict to an SdfLayer::FileFormatArguments
/// object and return it via \p args. 
///
/// If a non-string key or value is encountered, \p errMsg will be filled in
/// (if given) and this function will return false. Otherwise, this function
/// will return true.
SDF_API bool
SdfFileFormatArgumentsFromPython(
    const pxr_boost::python::dict& dict,
    SdfLayer::FileFormatArguments* args,
    std::string* errMsg = NULL);

SDF_NAMESPACE_CLOSE_SCOPE

#endif // PXR_SDF_PY_UTILS_H
