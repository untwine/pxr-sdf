// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

/// \file Allowed.cpp

#include "./allowed.h"

namespace pxr {

const std::string&
SdfAllowed::GetWhyNot() const
{
    static std::string empty;
    return _state ? *_state : empty;
}

}  // namespace pxr
