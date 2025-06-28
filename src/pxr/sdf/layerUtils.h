// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_LAYER_UTILS_H
#define PXR_SDF_LAYER_UTILS_H

/// \file sdf/layerUtils.h

#include "./api.h"
#include "./declareHandles.h"
#include "./layer.h"

#include <string>

namespace pxr {

SDF_DECLARE_HANDLES(SdfLayer);

/// Returns the path to the asset specified by \p assetPath, using the
/// \p anchor layer to anchor the path if it is relative.  If the result of
/// anchoring \p assetPath to \p anchor's path cannot be resolved and
/// \p assetPath is a search path, \p assetPath will be returned.  If
/// \p assetPath is not relative, \p assetPath will be returned.  Otherwise,
/// the anchored path will be returned.
///
/// Note that if \p anchor is an anonymous layer, we will always return
/// the untouched \p assetPath.
SDF_API std::string
SdfComputeAssetPathRelativeToLayer(
    const SdfLayerHandle& anchor,
    const std::string& assetPath);

}  // namespace pxr

#endif // PXR_SDF_LAYER_UTILS_H
