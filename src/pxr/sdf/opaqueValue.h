// Copyright 2022 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_OPAQUE_VALUE_H
#define PXR_SDF_OPAQUE_VALUE_H

#include "./api.h"

#include <cstddef>
#include <iosfwd>


namespace pxr {

/// In-memory representation of the value of an opaque attribute.
///
/// Opaque attributes cannot have authored values, but every typename in Sdf
/// must have a corresponding constructable C++ value type; SdfOpaqueValue is
/// the type associated with opaque attributes. Opaque values intentionally
/// cannot hold any information, cannot be parsed, and cannot be serialized to
/// a layer.
///
/// SdfOpaqueValue is also the type associated with group attributes. A group 
/// attribute is an opaque attribute that represents a group of other 
/// properties.
///
class SdfOpaqueValue final {};

inline bool
operator==(SdfOpaqueValue const &, SdfOpaqueValue const &)
{
    return true;
}

inline bool
operator!=(SdfOpaqueValue const &, SdfOpaqueValue const &)
{
    return false;
}

inline size_t hash_value(SdfOpaqueValue const &)
{
    // Use a nonzero constant here because some bad hash functions don't deal
    // with zero well. Chosen by fair dice roll.
    return 9;
}

SDF_API std::ostream& operator<<(std::ostream &, SdfOpaqueValue const &);

}  // namespace pxr

#endif
