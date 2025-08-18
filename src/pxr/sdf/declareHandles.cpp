// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/declareHandles.h"
#include "pxr/sdf/layer.h"
#include "pxr/sdf/specType.h"

SDF_NAMESPACE_OPEN_SCOPE

bool 
Sdf_CanCastToType(
    const SdfSpec& spec, const std::type_info& destType)
{
    return Sdf_SpecType::CanCast(spec.GetSpecType(), destType);
}

bool 
Sdf_CanCastToTypeCheckSchema(
    const SdfSpec& spec, const std::type_info& destType)
{
    return Sdf_SpecType::CanCast(spec, destType);
}

template <>
SdfHandleTo<SdfLayer>::Handle
SdfCreateHandle(SdfLayer *p)
{
    return SdfHandleTo<SdfLayer>::Handle(p);
}

SDF_NAMESPACE_CLOSE_SCOPE
