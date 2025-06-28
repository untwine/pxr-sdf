// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/layerTree.h>
#include <pxr/tf/makePyConstructor.h>
#include <pxr/tf/pyContainerConversions.h>
#include <pxr/tf/pyPtrHelpers.h>
#include <pxr/tf/pyResultConversions.h>
#include <pxr/boost/python.hpp>

using namespace pxr;

using namespace pxr::boost::python;

namespace {

static SdfLayerTreeHandle
_NewEmpty()
{
   SdfLayerTreeHandleVector childTrees;
   return SdfLayerTree::New(SdfLayerHandle(), childTrees);
}

static SdfLayerTreeHandle
_NewNoOffset(const SdfLayerHandle & layer,
             const SdfLayerTreeHandleVector & childTrees)
{
   return SdfLayerTree::New(layer, childTrees);
}

static SdfLayerTreeHandle
_New(const SdfLayerHandle & layer,
     const SdfLayerTreeHandleVector & childTrees,
     const SdfLayerOffset & cumulativeOffset)
{
   return SdfLayerTree::New(layer, childTrees, cumulativeOffset);
}

} // anonymous namespace 

void wrapLayerTree()
{    
    // Register conversion for python list <-> SdfLayerTreeHandleVector
    to_python_converter<SdfLayerTreeHandleVector,
                        TfPySequenceToPython<SdfLayerTreeHandleVector> >();
    TfPyContainerConversions::from_python_sequence<
        SdfLayerTreeHandleVector,
        TfPyContainerConversions::
            variable_capacity_all_items_convertible_policy >();

    class_<SdfLayerTree, TfWeakPtr<SdfLayerTree>, noncopyable>
        ("LayerTree", "", no_init)
        .def(TfPyRefAndWeakPtr())
        .def(TfMakePyConstructor(&_NewEmpty))
        .def(TfMakePyConstructor(&_NewNoOffset))
        .def(TfMakePyConstructor(&_New))
        .add_property("layer",
                      make_function(&SdfLayerTree::GetLayer,
                                    return_value_policy<return_by_value>()))
        .add_property("offset",
                      make_function(&SdfLayerTree::GetOffset,
                                    return_value_policy<return_by_value>()))
        .add_property("childTrees",
            make_function(&SdfLayerTree::GetChildTrees,
                          return_value_policy<TfPySequenceToList>()))
        ;
}
