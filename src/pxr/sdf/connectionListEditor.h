// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_CONNECTION_LIST_EDITOR_H
#define PXR_SDF_CONNECTION_LIST_EDITOR_H

#include "./listOpListEditor.h"
#include "./childrenPolicies.h"
#include "./declareHandles.h"
#include "./proxyPolicies.h"
#include "./types.h"

namespace pxr {

SDF_DECLARE_HANDLES(SdfSpec);
class TfToken;

/// \class Sdf_ConnectionListEditor
///
/// List editor implementation that ensures that the appropriate target
/// specs are created or destroyed when connection/relationship targets are 
/// added to the underlying list operation.
///
template <class ConnectionChildPolicy>
class Sdf_ConnectionListEditor 
    : public Sdf_ListOpListEditor<SdfPathKeyPolicy>
{
protected:
    virtual ~Sdf_ConnectionListEditor();

    Sdf_ConnectionListEditor(
        const SdfSpecHandle& connectionOwner,
        const TfToken& connectionListField,             
        const SdfPathKeyPolicy& typePolicy = SdfPathKeyPolicy());
    
    void _OnEditShared(SdfListOpType op,
        SdfSpecType specType,
        const std::vector<SdfPath>& oldItems,
        const std::vector<SdfPath>& newItems) const;

private:
    typedef Sdf_ListOpListEditor<SdfPathKeyPolicy> Parent;
};

/// \class Sdf_AttributeConnectionListEditor
///
/// List editor implementation for attribute connections.
///
class Sdf_AttributeConnectionListEditor
    : public Sdf_ConnectionListEditor<Sdf_AttributeConnectionChildPolicy>
{
public:
    virtual ~Sdf_AttributeConnectionListEditor();

    Sdf_AttributeConnectionListEditor(
        const SdfSpecHandle& owner,
        const SdfPathKeyPolicy& typePolicy = SdfPathKeyPolicy());

    virtual void _OnEdit(SdfListOpType op,
        const std::vector<SdfPath>& oldItems,
        const std::vector<SdfPath>& newItems) const;
private:
    typedef Sdf_ConnectionListEditor<Sdf_AttributeConnectionChildPolicy> Parent;
};

/// \class Sdf_RelationshipTargetListEditor
///
/// List editor implementation for relationship targets.
///
class Sdf_RelationshipTargetListEditor
    : public Sdf_ConnectionListEditor<Sdf_RelationshipTargetChildPolicy>
{
public:
    virtual ~Sdf_RelationshipTargetListEditor();

    Sdf_RelationshipTargetListEditor(
        const SdfSpecHandle& owner,
        const SdfPathKeyPolicy& typePolicy = SdfPathKeyPolicy());

    virtual void _OnEdit(SdfListOpType op,
        const std::vector<SdfPath>& oldItems,
        const std::vector<SdfPath>& newItems) const;
private:
    typedef Sdf_ConnectionListEditor<Sdf_RelationshipTargetChildPolicy> Parent;
};

}  // namespace pxr

#endif // PXR_SDF_CONNECTION_LIST_EDITOR_H
