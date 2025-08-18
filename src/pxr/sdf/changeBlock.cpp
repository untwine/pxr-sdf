// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/changeBlock.h"
#include "pxr/sdf/changeManager.h"

SDF_NAMESPACE_OPEN_SCOPE

SdfChangeBlock::SdfChangeBlock()
    : _key(Sdf_ChangeManager::Get()._OpenChangeBlock(this))
{
}

void
SdfChangeBlock::_CloseChangeBlock(void const *key) const
{
    Sdf_ChangeManager::Get()._CloseChangeBlock(this, key);
}

SDF_NAMESPACE_CLOSE_SCOPE
