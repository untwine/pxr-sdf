// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/variableExpression.h>

#include <pxr/tf/pyResultConversions.h>
#include <pxr/tf/pyUtils.h>
#include <pxr/vt/value.h>

#include <pxr/boost/python/class.hpp>
#include <pxr/boost/python/list.hpp>
#include <pxr/boost/python/scope.hpp>
#include <pxr/boost/python/tuple.hpp>

#include <string>
#include <vector>

using namespace pxr;

using namespace pxr::boost::python;

void
wrapVariableExpression()
{
    using This = SdfVariableExpression;

    scope s = class_<This>("VariableExpression")
        .def(init<>())
        .def(init<const std::string&>(arg("expression")))

        .def("__bool__", &This::operator bool)
        .def("__str__", &This::GetString,
            return_value_policy<return_by_value>())
        .def("__repr__", 
            +[](const This& expr) {
                return TfStringPrintf("%sVariableExpression('%s')",
                    TF_PY_REPR_PREFIX.c_str(), expr.GetString().c_str());
            })

        .def("GetErrors", &This::GetErrors,
            return_value_policy<TfPySequenceToList>())

        .def("Evaluate", &This::Evaluate,
            arg("vars"))

        .def("IsExpression", &This::IsExpression)
        .staticmethod("IsExpression")

        .def("IsValidVariableType", &This::IsValidVariableType)
        .staticmethod("IsValidVariableType")
        ;

    class_<This::Result>("Result", no_init)
        .add_property("value", 
            +[](const This::Result& r) {
                return r.value.IsHolding<This::EmptyList>() ?
                    object(list()) : object(r.value);
            })
        .add_property("errors", 
            make_getter(
                &This::Result::errors,
                return_value_policy<TfPySequenceToList>()))
        .add_property("usedVariables", 
            make_getter(
                &This::Result::usedVariables,
                return_value_policy<TfPySequenceToSet>()))
        ;
}
