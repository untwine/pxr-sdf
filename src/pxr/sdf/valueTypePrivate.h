// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_VALUE_TYPE_PRIVATE_H
#define PXR_SDF_VALUE_TYPE_PRIVATE_H

#include "./valueTypeName.h"
#include <pxr/tf/enum.h>
#include <pxr/tf/type.h>
#include <pxr/vt/value.h>
#include <vector>

namespace pxr {

class Sdf_ValueTypeImpl;

struct Sdf_ValueTypePrivate {
public:
    struct Empty { };

    // Represents a type/role pair.
    struct CoreType {
        CoreType() { }
        CoreType(Empty);

        TfType type;
        std::string cppTypeName;
        TfToken role;
        SdfTupleDimensions dim;
        VtValue value;
        TfEnum unit;

        // All type names aliasing this type/role pair in registration order.
        // The first alias is the "fundamental" type name.
        std::vector<TfToken> aliases;
    };

    /// Construct a SdfValueTypeName.
    static SdfValueTypeName MakeValueTypeName(const Sdf_ValueTypeImpl* impl);

    /// Return the value type implementation representing the empty type name.
    static const Sdf_ValueTypeImpl* GetEmptyTypeName();
};

/// Represents a registered type name.
class Sdf_ValueTypeImpl {
public:
    Sdf_ValueTypeImpl();

    const Sdf_ValueTypePrivate::CoreType* type;
    TfToken name;
    const Sdf_ValueTypeImpl* scalar;
    const Sdf_ValueTypeImpl* array;
};

}  // namespace pxr

#endif // PXR_SDF_VALUE_TYPE_PRIVATE_H
