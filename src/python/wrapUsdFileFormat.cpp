// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/pxr.h>

#include <pxr/sdf/usdFileFormat.h>
#include <pxr/sdf/layer.h>
#include <pxr/tf/pyStaticTokens.h>

#include <pxr/boost/python/bases.hpp>
#include <pxr/boost/python/class.hpp>
#include <pxr/boost/python/scope.hpp>

SDF_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void
wrapUsdFileFormat()
{
    using This = SdfUsdFileFormat;

    scope s = class_<This, bases<SdfFileFormat>, noncopyable>
        ("UsdFileFormat", no_init)

        .def("GetUnderlyingFormatForLayer", 
            &This::GetUnderlyingFormatForLayer)
        .staticmethod("GetUnderlyingFormatForLayer")
        ;

    TF_PY_WRAP_PUBLIC_TOKENS(
        "Tokens",
        SdfUsdFileFormatTokens,
        SDF_USD_FILE_FORMAT_TOKENS);
}
