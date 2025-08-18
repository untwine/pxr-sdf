// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_SITE_UTILS_H
#define PXR_SDF_SITE_UTILS_H

/// \file sdf/siteUtils.h
///
/// Convenience API for working with SdfSite.
///
/// These functions simply forward to the indicated functions on SdfLayer.

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/site.h"
#include "pxr/sdf/layer.h"
#include "pxr/sdf/primSpec.h"
#include "pxr/sdf/propertySpec.h"
#include "pxr/sdf/spec.h"

SDF_NAMESPACE_OPEN_SCOPE

inline
SdfSpecHandle
SdfGetObjectAtPath(const SdfSite& site)
{
    return site.layer->GetObjectAtPath(site.path);
}

inline
SdfPrimSpecHandle
SdfGetPrimAtPath(const SdfSite& site)
{
    return site.layer->GetPrimAtPath(site.path);
}

inline
SdfPropertySpecHandle
SdfGetPropertyAtPath(const SdfSite& site)
{
    return site.layer->GetPropertyAtPath(site.path);
}

inline 
bool
SdfHasField(const SdfSite& site, const TfToken& field)
{
    return site.layer->HasField(site.path, field);
}

template <class T>
inline bool 
SdfHasField(const SdfSite& site, const TfToken& field, T* value)
{
    return site.layer->HasField(site.path, field, value);
}

inline
const VtValue
SdfGetField(const SdfSite& site, const TfToken& field)
{
    return site.layer->GetField(site.path, field);
}

template <class T>
inline 
T
SdfGetFieldAs(const SdfSite& site, const TfToken& field, 
              const T& defaultValue = T())
{
    return site.layer->GetFieldAs<T>(site.path, field, defaultValue);
}

SDF_NAMESPACE_CLOSE_SCOPE

#endif // PXR_SDF_SITE_UTILS_H
