// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

/// \file wrapVariantSpec.cpp

#include <pxr/sdf/variantSpec.h>
#include <pxr/sdf/primSpec.h>
#include <pxr/sdf/pySpec.h>
#include <pxr/sdf/variantSetSpec.h>
#include <pxr/sdf/pyChildrenProxy.h>
#include <pxr/boost/python.hpp>

using namespace pxr;

using namespace pxr::boost::python;

namespace {

typedef SdfPyChildrenProxy<SdfVariantSetView> VariantSetProxy;

static
VariantSetProxy
_WrapGetVariantSetsProxy(const SdfVariantSpec& owner)
{
    return VariantSetProxy(owner.GetVariantSets());
}

} // anonymous namespace 

void wrapVariantSpec()
{
    def("CreateVariantInLayer", SdfCreateVariantInLayer);

    typedef SdfVariantSpec This;

    class_<This, SdfHandle<This>, bases<SdfSpec>, noncopyable>
        ("VariantSpec", no_init)
        .def(SdfPySpec())
        .def(SdfMakePySpecConstructor(&This::New))

        .add_property("primSpec", &This::GetPrimSpec,
            "The root prim of this variant.")
        .add_property("owner", &This::GetOwner,
            "The variant set that this variant belongs to.")
        .add_property("name",
            make_function(&This::GetName,
                          return_value_policy<return_by_value>()),
            "The variant's name.")
        .add_property("variantSets",
            &_WrapGetVariantSetsProxy)
        .def("GetVariantNames", &This::GetVariantNames)
        ;
}
