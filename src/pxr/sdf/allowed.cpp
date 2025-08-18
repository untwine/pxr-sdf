// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

/// \file Allowed.cpp

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/allowed.h"

SDF_NAMESPACE_OPEN_SCOPE

const std::string&
SdfAllowed::GetWhyNot() const
{
    static std::string empty;
    return _state ? *_state : empty;
}

SDF_NAMESPACE_CLOSE_SCOPE
