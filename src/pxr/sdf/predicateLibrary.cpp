// Copyright 2023 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./predicateLibrary.h"

#include <pxr/tf/diagnostic.h>
#include <pxr/tf/errorMark.h>

namespace pxr {

TF_REGISTRY_FUNCTION(pxr::TfEnum)
{
    // SdfPredicateFunctionResult::Constancy
    TF_ADD_ENUM_NAME(SdfPredicateFunctionResult::ConstantOverDescendants);
    TF_ADD_ENUM_NAME(SdfPredicateFunctionResult::MayVaryOverDescendants);
}

size_t
SdfPredicateParamNamesAndDefaults::_CountDefaults() const
{
    size_t count = 0;
    for (Param const &p: _params) {
        if (!p.val.IsEmpty()) {
            ++count;
        }
    }
    return count;
}

bool
SdfPredicateParamNamesAndDefaults::CheckValidity() const
{
    // Basic check -- names cannot be empty, once an arg with a default appears,
    // all subsequent args must have defaults.
    TfErrorMark m;
    Param const *firstDefault = nullptr;
    for (Param const &param: _params) {
        if (param.name.empty()) {
            TF_CODING_ERROR(
                "Specified empty predicate expression parameter name");
        }
        bool hasDefault = !param.val.IsEmpty();
        if (firstDefault) {
            if (!hasDefault) {
                TF_CODING_ERROR("Non-default predicate function parameter "
                                "'%s' follows default parameter '%s'",
                                param.name.c_str(),
                                firstDefault->name.c_str());
            }
        }
        else if (hasDefault) {
            firstDefault = &param;
        }
    }
    return m.IsClean();
}

}  // namespace pxr
