// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

/// \file wrapPseudoRootSpec.cpp

#include <pxr/sdf/pxr.h>
#include <pxr/sdf/pseudoRootSpec.h>
#include <pxr/sdf/pySpec.h>

#include <pxr/boost/python.hpp>

SDF_NAMESPACE_USING_DIRECTIVE

using namespace pxr_boost::python;

void
wrapPseudoRootSpec()
{
    typedef SdfPseudoRootSpec This;

    class_<This, SdfHandle<This>, 
           bases<SdfPrimSpec>, noncopyable>
        ("PseudoRootSpec", no_init)
        .def(SdfPySpec())
        ;
}
