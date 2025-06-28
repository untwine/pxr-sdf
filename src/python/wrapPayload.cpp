// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/payload.h>
#include <pxr/tf/pyContainerConversions.h>
#include <pxr/tf/pyUtils.h>
#include <pxr/vt/valueFromPython.h>

#include <pxr/boost/python.hpp>

#include <string>

using std::string;

using namespace pxr;

using namespace pxr::boost::python;

namespace {

static string
_Repr(const SdfPayload &self)
{
    string args;
    bool useKeywordArgs = false;

    if (!self.GetAssetPath().empty()) {
        args += TfPyRepr(self.GetAssetPath());
    } else {
        useKeywordArgs = true;
    }
    if (!self.GetPrimPath().IsEmpty()) {
        args += (args.empty() ? "": ", ");
        args += (useKeywordArgs ? "primPath=" : "") +
            TfPyRepr(self.GetPrimPath());
    } else {
        useKeywordArgs = true;
    }
    if (!self.GetLayerOffset().IsIdentity()) {
        args += (args.empty() ? "": ", ");
        args += (useKeywordArgs ? "layerOffset=" : "") +
            TfPyRepr(self.GetLayerOffset());
    } else {
        useKeywordArgs = true;
    }

    return TF_PY_REPR_PREFIX + "Payload(" + args + ")";
}

static size_t __hash__(const SdfPayload &self)
{
    return TfHash()(self);
}

} // anonymous namespace 

void wrapPayload()
{    
    typedef SdfPayload This;

    class_<This>( "Payload" )
        .def(init<const string &,
                  const SdfPath &,
                  const SdfLayerOffset &>(
            ( arg("assetPath") = string(),
              arg("primPath") = SdfPath(),
              arg("layerOffset") = SdfLayerOffset() ) ) )
        .def(init<const This &>())

        .add_property("assetPath",
            make_function(
                &This::GetAssetPath, return_value_policy<return_by_value>()),
            &This::SetAssetPath)

        .add_property("primPath",
            make_function(
                &This::GetPrimPath, return_value_policy<return_by_value>()),
            &This::SetPrimPath)

        .add_property("layerOffset",
            make_function(
                &This::GetLayerOffset, return_value_policy<return_by_value>()),
            &This::SetLayerOffset)

        .def(self == self)
        .def(self != self)
        .def(self < self)
        .def(self > self)
        .def(self <= self)
        .def(self >= self)

        .def("__repr__", _Repr)
        .def("__hash__", __hash__)

        ;

    VtValueFromPython<SdfPayload>();

    // Register conversion for python list <-> vector<SdfPayload>
    to_python_converter<
        SdfPayloadVector,
        TfPySequenceToPython<SdfPayloadVector> >();
    TfPyContainerConversions::from_python_sequence<
        SdfPayloadVector,
        TfPyContainerConversions::variable_capacity_policy >();
}
