// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_SITE_H
#define PXR_SDF_SITE_H

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/layer.h"
#include "pxr/sdf/path.h"

#include <set>
#include <vector>

SDF_NAMESPACE_OPEN_SCOPE

/// \class SdfSite
///
/// An SdfSite is a simple representation of a location in a layer where 
/// opinions may possibly be found. It is simply a pair of layer and path
/// within that layer.
///
class SdfSite
{
public:
    SdfSite() { }
    SdfSite(const SdfLayerHandle& layer_, const SdfPath& path_)
        : layer(layer_)
        , path(path_)
    { }

    bool operator==(const SdfSite& other) const
    {
        return layer == other.layer && path == other.path;
    }

    bool operator!=(const SdfSite& other) const
    {
        return !(*this == other);
    }

    bool operator<(const SdfSite& other) const
    {
        return layer < other.layer ||
               (!(other.layer < layer) && path < other.path);
    }

    bool operator>(const SdfSite& other) const
    {
        return other < *this;
    }

    bool operator<=(const SdfSite& other) const
    {
        return !(other < *this);
    }

    bool operator>=(const SdfSite& other) const
    {
        return !(*this < other);
    }

    /// Explicit bool conversion operator. A site object converts to \c true iff 
    /// both the layer and path fields are filled with valid values, \c false
    /// otherwise.
    /// This does NOT imply that there are opinions in the layer at that path.
    explicit operator bool() const
    {
        return layer && !path.IsEmpty();
    }

public:
    SdfLayerHandle layer;
    SdfPath path;
};

typedef std::set<SdfSite> SdfSiteSet;
typedef std::vector<SdfSite> SdfSiteVector;

SDF_NAMESPACE_CLOSE_SCOPE

#endif // PXR_SDF_SITE_H
