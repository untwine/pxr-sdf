// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_LAYER_UTILS_H
#define PXR_SDF_LAYER_UTILS_H

/// \file sdf/layerUtils.h

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/api.h"
#include "pxr/sdf/declareHandles.h"
#include "pxr/sdf/layer.h"

#include <string>

SDF_NAMESPACE_OPEN_SCOPE

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

/// Wrapper for SdfComputeAssetPathRelativeToLayer that returns the resolved 
/// \p assetPath. If \p assetPath is empty or \p anchor is anonymous, \p assetPath
/// returns unchanged.
SDF_API std::string
SdfResolveAssetPathRelativeToLayer(
    const SdfLayerHandle& anchor,
    const std::string& assetPath);

SDF_NAMESPACE_CLOSE_SCOPE

#endif // PXR_SDF_LAYER_UTILS_H
