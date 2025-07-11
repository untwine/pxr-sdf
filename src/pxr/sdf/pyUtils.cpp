// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./pyUtils.h"
#include <pxr/boost/python/extract.hpp>
#include <pxr/boost/python/object.hpp>

namespace pxr {

bool
SdfFileFormatArgumentsFromPython(
    const pxr::boost::python::dict& dict,
    SdfLayer::FileFormatArguments* args,
    std::string* errMsg)
{
    SdfLayer::FileFormatArguments argsMap;
    typedef SdfLayer::FileFormatArguments::key_type ArgKeyType;
    typedef SdfLayer::FileFormatArguments::mapped_type ArgValueType;

    const pxr::boost::python::object items = dict.items();
    for (pxr::boost::python::ssize_t i = 0; i < len(items); ++i) {
        pxr::boost::python::extract<ArgKeyType> keyExtractor(items[i][0]);
        if (!keyExtractor.check()) {
            if (errMsg) {
                *errMsg = "All file format argument keys must be strings";
            }
            return false;
        }

        pxr::boost::python::extract<ArgValueType> valueExtractor(items[i][1]);
        if (!valueExtractor.check()) {
            if (errMsg) {
                *errMsg = "All file format argument values must be strings";
            }
            return false;
        }
            
        argsMap[keyExtractor()] = valueExtractor();
    }

    args->swap(argsMap);
    return true;
}

}  // namespace pxr
