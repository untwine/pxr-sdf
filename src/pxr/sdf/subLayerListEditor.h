// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_SUB_LAYER_LIST_EDITOR_H
#define PXR_SDF_SUB_LAYER_LIST_EDITOR_H

/// \file sdf/subLayerListEditor.h

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/vectorListEditor.h"
#include "pxr/sdf/declareHandles.h"
#include "pxr/sdf/proxyPolicies.h"

SDF_NAMESPACE_OPEN_SCOPE

SDF_DECLARE_HANDLES(SdfLayer);

/// \class Sdf_SubLayerListEditor
///
/// List editor implementation for sublayer path lists.
///
class Sdf_SubLayerListEditor 
    : public Sdf_VectorListEditor<SdfSubLayerTypePolicy>
{
public:
    Sdf_SubLayerListEditor(const SdfLayerHandle& owner);

    virtual ~Sdf_SubLayerListEditor();

private:
    typedef Sdf_VectorListEditor<SdfSubLayerTypePolicy> Parent;

    virtual void _OnEdit(
        SdfListOpType op,
        const std::vector<std::string>& oldValues,
        const std::vector<std::string>& newValues) const;
};

SDF_NAMESPACE_CLOSE_SCOPE

#endif // PXR_SDF_SUB_LAYER_LIST_EDITOR_H
