// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_LAYER_TREE_H
#define PXR_SDF_LAYER_TREE_H

/// \file sdf/layerTree.h

#include "./api.h"
#include "./declareHandles.h"
#include "./layerOffset.h"

#include <vector>

namespace pxr {

// Layer tree forward declarations.
class SdfLayerTree;
typedef TfRefPtr<SdfLayerTree> SdfLayerTreeHandle;
typedef std::vector<SdfLayerTreeHandle> SdfLayerTreeHandleVector;

SDF_DECLARE_HANDLES(SdfLayer);

/// \class SdfLayerTree
///
/// A SdfLayerTree is an immutable tree structure representing a sublayer
/// stack and its recursive structure.
///
/// Layers can have sublayers, which can in turn have sublayers of their
/// own.  Clients that want to represent that hierarchical structure in
/// memory can build a SdfLayerTree for that purpose.
///
/// We use TfRefPtr<SdfLayerTree> as handles to LayerTrees, as a simple way
/// to pass them around as immutable trees without worrying about lifetime.
///
class SdfLayerTree : public TfRefBase, public TfWeakBase {
    SdfLayerTree(const SdfLayerTree&) = delete;
    SdfLayerTree& operator=(const SdfLayerTree&) = delete;
public:
    /// Create a new layer tree node.
    SDF_API
    static SdfLayerTreeHandle
    New( const SdfLayerHandle & layer,
         const SdfLayerTreeHandleVector & childTrees,
         const SdfLayerOffset & cumulativeOffset = SdfLayerOffset() );

    /// Returns the layer handle this tree node represents.
    SDF_API const SdfLayerHandle & GetLayer() const;

    /// Returns the cumulative layer offset from the root of the tree.
    SDF_API const SdfLayerOffset & GetOffset() const;

    /// Returns the children of this tree node.
    SDF_API const SdfLayerTreeHandleVector & GetChildTrees() const;

private:
    SdfLayerTree( const SdfLayerHandle & layer,
                  const SdfLayerTreeHandleVector & childTrees,
                  const SdfLayerOffset & cumulativeOffset );

private:
    const SdfLayerHandle _layer;
    const SdfLayerOffset _offset;
    const SdfLayerTreeHandleVector _childTrees;
};

}  // namespace pxr

#endif // PXR_SDF_LAYER_TREE_H
