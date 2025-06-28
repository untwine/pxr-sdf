// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/opaqueValue.h>

#include <pxr/tf/pyStaticTokens.h>
#include <pxr/vt/valueFromPython.h>

#include <pxr/boost/python.hpp>
#include <string>

using namespace pxr;

using namespace pxr::boost::python;


static std::string
_SdfOpaqueValueRepr(const SdfOpaqueValue &self)
{
    return TF_PY_REPR_PREFIX + "OpaqueValue()";
}

static size_t
_SdfOpaqueValueHash(const SdfOpaqueValue &self)
{
    return TfHash()(self);
}

void wrapOpaqueValue()
{
    class_<SdfOpaqueValue>("OpaqueValue")
        .def(self == self)
        .def(self != self)
        .def("__repr__", _SdfOpaqueValueRepr)
        .def("__hash__", _SdfOpaqueValueHash);
    VtValueFromPython<SdfOpaqueValue>();
}
