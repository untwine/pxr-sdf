// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_MAP_EDITOR_H
#define PXR_SDF_MAP_EDITOR_H

#include "./allowed.h"
#include "./spec.h"

#include <memory>
#include <string>
#include <utility>

namespace pxr {

class TfToken;

/// \class Sdf_MapEditor
///
/// Interface for private implementations used by SdfMapEditProxy.
///
template <class MapType>
class Sdf_MapEditor {
public:
    typedef typename MapType::key_type    key_type;
    typedef typename MapType::mapped_type mapped_type;
    typedef typename MapType::value_type  value_type;
    typedef typename MapType::iterator    iterator;

    virtual ~Sdf_MapEditor() noexcept;

    /// Returns a string describing the location of the map being edited.
    /// This is used for debugging and error messages.
    virtual std::string GetLocation() const = 0;

    /// Returns owner of the map being edited.
    virtual SdfSpecHandle GetOwner() const = 0;

    /// Returns true if the map being edited is expired, false otherwise.
    virtual bool IsExpired() const = 0;

    /// Returns const pointer to map being edited.
    virtual const MapType* GetData() const = 0;

    /// Returns non-const pointer to map being edited. 
    /// All edits to the map should be done using the editing functions below.
    /// This function is primarily here for convenience. Ideally, only the
    /// const version of this function would exist.
    virtual MapType* GetData() = 0;
    
    /// \name Editing Operations
    /// @{

    virtual void Copy(const MapType& other) = 0;
    virtual void Set(const key_type& key, const mapped_type& other) = 0;
    virtual std::pair<iterator, bool> Insert(const value_type& value) = 0;
    virtual bool Erase(const key_type& key) = 0;

    virtual SdfAllowed IsValidKey(const key_type& key) const = 0;
    virtual SdfAllowed IsValidValue(const mapped_type& value) const = 0;

    /// @}

protected:
    Sdf_MapEditor();

};

template <class T>
std::unique_ptr<Sdf_MapEditor<T> > 
Sdf_CreateMapEditor(const SdfSpecHandle& owner, const TfToken& field);

}  // namespace pxr

#endif // PXR_SDF_MAP_EDITOR_H
