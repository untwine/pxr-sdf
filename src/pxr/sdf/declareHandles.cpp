// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./declareHandles.h"
#include "./layer.h"
#include "./specType.h"

namespace pxr {

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

}  // namespace pxr
