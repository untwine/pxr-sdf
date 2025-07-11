// Copyright 2024 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/tf/pyEnum.h>

#include <pxr/sdf/predicateLibrary.h>

#include <pxr/boost/python/class.hpp>

using namespace pxr;

using namespace pxr::boost::python;

static std::string
_Repr(SdfPredicateFunctionResult const &self) {
    return TF_PY_REPR_PREFIX +
        "PredicateFunctionResult(" +
        TfPyRepr(self.GetValue()) + ", " +
        TfPyRepr(self.GetConstancy()) + ")";
}

static bool
_Bool(SdfPredicateFunctionResult const &self) {
    return self.GetValue();
}
                           
void wrapPredicateFunctionResult()
{
    using FunctionResult = SdfPredicateFunctionResult;
    
    scope s = class_<FunctionResult>("PredicateFunctionResult")
        .def(init<FunctionResult const &>())
        .def(init<bool, optional<FunctionResult::Constancy>>(
                 (arg("value"), arg("constancy"))))

        .def("MakeConstant", &FunctionResult::MakeConstant, arg("value"))
        .staticmethod("MakeConstant")
        .def("MakeVarying", &FunctionResult::MakeVarying, arg("value"))
        .staticmethod("MakeVarying")

        .def("GetValue", &FunctionResult::GetValue)
        .def("GetConstancy", &FunctionResult::GetConstancy)
        .def("IsConstant", &FunctionResult::IsConstant)

        .def("SetAndPropagateConstancy",
             &FunctionResult::SetAndPropagateConstancy)

        .def(!self)
        .def("__bool__", _Bool)
        .def(self == self)
        .def(self != self)
        .def(self == bool{})
        .def(bool{} == self)
        .def(self != bool{})
        .def(bool{} != self)

        .def("__repr__", _Repr)
        ;

    TfPyWrapEnum<FunctionResult::Constancy>();
}
