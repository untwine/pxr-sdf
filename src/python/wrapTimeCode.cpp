// Copyright 2019 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/timeCode.h>
#include <pxr/vt/valueFromPython.h>
#include <pxr/tf/hash.h>
#include <pxr/tf/pyResultConversions.h>
#include <pxr/tf/stringUtils.h>
#include <pxr/vt/wrapArray.h>

#include <pxr/boost/python/class.hpp>
#include <pxr/boost/python/def.hpp>
#include <pxr/boost/python/implicit.hpp>
#include <pxr/boost/python/operators.hpp>

#include <sstream>

using namespace pxr;

using namespace pxr::boost::python;

TF_REGISTRY_FUNCTION(pxr::VtValue)
{
    VtRegisterValueCastsFromPythonSequencesToArray<SdfTimeCode>();
}

namespace {

static std::string _Str(SdfTimeCode const &self)
{
    return TfStringify(self);
}

static std::string
_Repr(SdfTimeCode const &self)
{
    std::ostringstream repr;
    repr << TF_PY_REPR_PREFIX << "TimeCode(" << self << ")";
    return repr.str();
}

static bool _HasNonZeroTimeCode(SdfTimeCode const &self)
{
    return self != SdfTimeCode(0.0);
}

static double _Float(SdfTimeCode const &self)
{
    return double(self);
}

} // anonymous namespace 

void wrapTimeCode()
{
    typedef SdfTimeCode This;

    auto selfCls = class_<This>("TimeCode", init<>())
        .def(init<double>())

        .def("GetValue", &This::GetValue)

        .def("__repr__", _Repr)
        .def("__str__", _Str)
        .def("__bool__", _HasNonZeroTimeCode)
        .def("__hash__", &This::GetHash)
        .def("__float__", _Float)

        .def( self == self )
        .def( double() == self )
        .def( self != self )
        .def( double() != self )
        .def( self < self )
        .def( double() < self )
        .def( self > self )
        .def( double() > self )
        .def( self <= self )
        .def( double() <= self )
        .def( self >= self )
        .def( double() >= self )

        .def( self * self )
        .def( double() * self )
        .def( self / self )
        .def( double() / self )
        .def( self + self )
        .def( double() + self )
        .def( self - self )
        .def( double() - self )
        ;

    implicitly_convertible<double, This>();

    // Let python know about us, to enable assignment from python back to C++
    VtValueFromPython<SdfTimeCode>();
}
