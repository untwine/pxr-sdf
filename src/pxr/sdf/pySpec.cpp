// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

/// \file PySpec.cpp

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/pySpec.h"
#include "pxr/sdf/layer.h"
#include "pxr/sdf/spec.h"
#include "pxr/sdf/specType.h"
#include <pxr/arch/demangle.h>
#include <pxr/tf/diagnostic.h>
#include <pxr/tf/pyUtils.h>
#include <pxr/tf/staticData.h>
#include <pxr/tf/type.h>

SDF_NAMESPACE_OPEN_SCOPE

namespace Sdf_PySpecDetail {

bp::object
_DummyInit(bp::tuple const & /* args */, bp::dict const & /* kw */)
{
    return bp::object();
}

// Returns a repr based on Sdf.Find().
std::string
_SpecRepr(const bp::object& self, const SdfSpec* spec)
{
    if (!spec || spec->IsDormant() || !spec->GetLayer()) {
        return "<dormant " + TfPyGetClassName(self) + ">";
    }
    else {
        SdfLayerHandle layer = spec->GetLayer();
        std::string path = layer->GetIdentifier();
        return TF_PY_REPR_PREFIX + "Find(" +
               TfPyRepr(path) +
               ", " +
               TfPyRepr(spec->GetPath().GetString()) +
               ")";
    }
}

typedef std::map<TfType, _HolderCreator> _HolderCreatorMap;
static TfStaticData<_HolderCreatorMap> _holderCreators;

void
_RegisterHolderCreator(const std::type_info& ti, _HolderCreator creator)
{
    TfType type = TfType::Find(ti);
    if (type.IsUnknown()) {
        TF_CODING_ERROR("No TfType registered for type \"%s\"",
                        ArchGetDemangled(ti).c_str());
    }
    else if (!_holderCreators->insert(std::make_pair(type, creator)).second){
        TF_CODING_ERROR("Duplicate conversion for \"%s\" ignored",
                        type.GetTypeName().c_str());
    }
}

PyObject*
_CreateHolder(const std::type_info& ti, const SdfSpec& spec)
{
    if (spec.IsDormant()) {
        return bp::detail::none();
    }
    else {
        // Get the TfType for the object's actual type.  If there's an
        // ambiguity (e.g. for SdfVariantSpec) then use type ti.
        TfType type = Sdf_SpecType::Cast(spec, ti);

        // Find the creator for the type and invoke it.
        _HolderCreatorMap::const_iterator i = _holderCreators->find(type);
        if (i == _holderCreators->end()) {
            if (!type.IsUnknown()) {
                TF_CODING_ERROR("No conversion for registed for \"%s\"",
                                type.GetTypeName().c_str());
            }
            return bp::detail::none();
        }
        else {
            return (i->second)(spec);
        }
    }
}

}

SDF_NAMESPACE_CLOSE_SCOPE
