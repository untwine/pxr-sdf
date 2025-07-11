// Copyright 2020 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_LAYER_HINTS_H
#define PXR_SDF_LAYER_HINTS_H

namespace pxr {

/// Contains hints about layer contents that may be used to accelerate certain
/// composition operations.
class SdfLayerHints
{
public:
    /// Default constructed hints provide the most conservative set of values
    /// such that consumers of the hints will act correctly if not optimally.
    SdfLayerHints() = default;

    /// Construct hints with specific values.  Using this constructor requires
    /// that all hint fields be specified.
    explicit SdfLayerHints(bool mightHaveRelocates)
        : mightHaveRelocates(mightHaveRelocates)
    {}

    /// If this field is false, the layer does not contain relocates.  If
    /// true, relocates may be present but are not guaranteed to exist.
    bool mightHaveRelocates = true;
};

}  // namespace pxr

#endif
