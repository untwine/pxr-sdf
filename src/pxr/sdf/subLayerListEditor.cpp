// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

/// \file SubLayerListEditor.cpp

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/subLayerListEditor.h"
#include "pxr/sdf/layer.h"
#include "pxr/sdf/layerOffset.h"
#include "pxr/sdf/primSpec.h"
#include <pxr/vt/dictionary.h>
#include <pxr/tf/mallocTag.h>

SDF_NAMESPACE_OPEN_SCOPE

Sdf_SubLayerListEditor::Sdf_SubLayerListEditor(
    const SdfLayerHandle& owner)
    : Parent(owner->GetPseudoRoot(), 
             SdfFieldKeys->SubLayers, SdfListOpTypeOrdered)
{
}

Sdf_SubLayerListEditor::~Sdf_SubLayerListEditor() = default;

void 
Sdf_SubLayerListEditor::_OnEdit(
    SdfListOpType op,
    const std::vector<std::string>& oldValues,
    const std::vector<std::string>& newValues) const
{
    // When sublayer paths are added or removed, we need to keep the
    // sublayer offsets vector (stored in a separate field) in sync.
    const SdfLayerOffsetVector oldLayerOffsets = _GetOwner()->
        GetFieldAs<SdfLayerOffsetVector>(SdfFieldKeys->SubLayerOffsets);

    // If this is ever the case, bad things will probably happen as code
    // in SdfLayer assumes the two vectors are in sync.
    if (!TF_VERIFY(oldValues.size() == oldLayerOffsets.size(),
                  "Sublayer offsets do not match sublayer paths")) {
        return;
    }

    // Rebuild the layer offsets vector, retaining offsets.
    SdfLayerOffsetVector newLayerOffsets(newValues.size());
    for (size_t i = 0; i < newValues.size(); ++i) {
        const std::string& newLayer = newValues[i];

        std::vector<std::string>::const_iterator oldValuesIt = 
            std::find(oldValues.begin(), oldValues.end(), newLayer);
        if (oldValuesIt == oldValues.end()) {
            continue;
        }
        
        const size_t oldLayerOffsetIndex = 
            std::distance(oldValues.begin(), oldValuesIt);
        newLayerOffsets[i] = oldLayerOffsets[oldLayerOffsetIndex];
    }
    
    _GetOwner()->SetField(SdfFieldKeys->SubLayerOffsets, newLayerOffsets);
}

SDF_NAMESPACE_CLOSE_SCOPE
