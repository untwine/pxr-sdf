// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

/// \file wrapSpec.cpp

#include <pxr/sdf/spec.h>
#include <pxr/sdf/path.h>
#include <pxr/sdf/pySpec.h>
#include <pxr/sdf/schema.h>
#include <pxr/sdf/types.h>
#include <pxr/tf/pyResultConversions.h>
#include <pxr/tf/ostreamMethods.h>

#include <pxr/boost/python/class.hpp>

using namespace pxr;

using namespace pxr::boost::python;

namespace {

static
VtValue
_WrapGetInfo(SdfSpec &self, const TfToken &name)
{
    return self.GetInfo(name);
}

static
bool
_WrapIsInertProperty(SdfSpec &self)
{
    return self.IsInert();
}

static
void
_WrapSetInfo(SdfSpec &self, const TfToken &name, const object& pyObj)
{
    VtValue fallback;
    if (!self.GetSchema().IsRegistered(name, &fallback)) {
        TF_CODING_ERROR("Invalid info key: %s", name.GetText());
        return;
    }

    VtValue value;
    if (fallback.IsEmpty()) {
        value = extract<VtValue>(pyObj)();
    }
    else {
        // We have to handle a few things as special cases to disambiguate
        // types from Python.
        if (fallback.IsHolding<SdfPath>()) {
            value = extract<SdfPath>(pyObj)();
        }
        else if (fallback.IsHolding<TfTokenVector>()) {
            value = extract<TfTokenVector>(pyObj)();
        }
        else if (fallback.IsHolding<SdfVariantSelectionMap>()) {
            value = extract<SdfVariantSelectionMap>(pyObj)();
        }
        else if (fallback.IsHolding<SdfRelocatesMap>()) {
            value = extract<SdfRelocatesMap>(pyObj)();
        }
        else if (fallback.IsHolding<SdfRelocates>()) {
            value = extract<SdfRelocates>(pyObj)();
        }
        else {
            value = extract<VtValue>(pyObj)();
            value.CastToTypeOf(fallback);
        }
    }
    
    if (value.IsEmpty()) {
        TfPyThrowTypeError("Invalid type for key");
        return;
    }

    self.SetInfo(name, value);
}

static
std::string
_GetAsText(const SdfSpecHandle &self)
{
    if (!self) {
        return TfPyRepr(self);
    }
    std::stringstream stream;
    self->WriteToStream(stream);
    return stream.str();
}

} // anonymous namespace 

void wrapSpec()
{
    typedef SdfSpec This;

    class_<This, SdfHandle<This>, noncopyable>("Spec", no_init)
        .def(SdfPyAbstractSpec())

        .add_property("layer", &This::GetLayer,
            "The owning layer.")
        .add_property("path", &This::GetPath,
            "The absolute scene path.")

        .def("GetAsText", &_GetAsText)

        .def("ListInfoKeys", &This::ListInfoKeys,
            return_value_policy<TfPySequenceToList>())
        .def("GetMetaDataInfoKeys", &This::GetMetaDataInfoKeys,
            return_value_policy<TfPySequenceToList>())

        .def("GetMetaDataDisplayGroup", &This::GetMetaDataDisplayGroup)

        .def("GetInfo", &_WrapGetInfo)
        .def("SetInfo", &_WrapSetInfo)
        .def("SetInfoDictionaryValue", &This::SetInfoDictionaryValue)
        .def("HasInfo", &This::HasInfo,
             "HasInfo(key) -> bool\n\n"

             "key : string\n\n"

             "Returns whether there is a setting for the scene spec "
             "info with the given key.\n\n"
            
             "When asked for a value for one of its scene spec info, a "
             "valid value will always be returned. But if this API returns "
             "false for a scene spec info, the value of that info will be "
             "the defined default value. \n\n"
             "(XXX: This may change such that it is an error to "
             "ask for a value when there is none).\n\n"
             
             "When dealing with a composedLayer, it is not necessary to worry "
             "about whether a scene spec info 'has a value' because the "
             "composed layer will always have a valid value, even if it is the "
             "default.\n\n"
            
             "A spec may or may not have an expressed value for "
             "some of its scene spec info.")

        .def("ClearInfo", &This::ClearInfo,
             "ClearInfo(key)\n\n"

             "key : string\nn"

             "Clears the value for scene spec info with the given key. "
             "After calling this, HasInfo() will return false. "
             "To make HasInfo() return true, set a value for that scene "
             "spec info.",
             (arg("key")))

        .def("GetTypeForInfo", &This::GetTypeForInfo,
             "GetTypeForInfo(key)\n\n"

             "key : string\n\n"

             "Returns the type of value for the given key. ")

        .def("GetFallbackForInfo",
              make_function(&This::GetFallbackForInfo,
                  return_value_policy<return_by_value>()),
             "GetFallbackForInfo(key)\n\n"

             "key : string\n\n"

             "Returns the fallback value for the given key. ")

        .add_property("isInert", &_WrapIsInertProperty,
              "Indicates whether this spec has any significant data. This is "
              "for backwards compatibility, use IsInert instead.\n\n"

              "Compatibility note: prior to presto 1.9, isInert (then isEmpty) "
              "was true for otherwise inert PrimSpecs with inert inherits, "
              "references, or variant sets. isInert is now false in such "
              "conditions.")

        .def("IsInert", &This::IsInert,
             (arg("ignoreChildren") = false),
             
             "Indicates whether this spec has any significant data. "
             "If ignoreChildren is true, child scenegraph objects will be "
             "ignored.")
       ;
}
