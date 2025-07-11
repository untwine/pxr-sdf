// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./mapEditor.h"
#include "./path.h"
#include "./schema.h"

#include <pxr/vt/value.h>
#include <pxr/tf/diagnostic.h>
#include <pxr/tf/mallocTag.h>

#include <utility>

namespace pxr {

//
// Sdf_MapEditor<T>
//

template <class T>
Sdf_MapEditor<T>::Sdf_MapEditor() = default;

template <class T>
Sdf_MapEditor<T>::~Sdf_MapEditor() noexcept = default;

//
// Sdf_LsdMapEditor<T>
//

template <class T>
class Sdf_LsdMapEditor : 
    public Sdf_MapEditor<T>
{
public:
    typedef typename Sdf_MapEditor<T>::key_type key_type;
    typedef typename Sdf_MapEditor<T>::mapped_type mapped_type;
    typedef typename Sdf_MapEditor<T>::value_type value_type;
    typedef typename Sdf_MapEditor<T>::iterator iterator;

    Sdf_LsdMapEditor(const SdfSpecHandle& owner, const TfToken& field) :
        _owner(owner),
        _field(field)
    {
        const VtValue& dataVal = _owner->GetField(_field);
        if (!dataVal.IsEmpty()) {
            if (dataVal.IsHolding<T>()) {
                _data = dataVal.Get<T>();
            }
            else {
                TF_CODING_ERROR("%s does not hold value of expected type.",
                                GetLocation().c_str());
            }
        }
    }

    virtual ~Sdf_LsdMapEditor() noexcept = default;

    virtual std::string GetLocation() const
    {
        return TfStringPrintf("field '%s' in <%s>", 
                              _field.GetText(), _owner->GetPath().GetText());
    }

    virtual SdfSpecHandle GetOwner() const
    {
        return _owner;
    }

    virtual bool IsExpired() const
    {
        return !_owner;
    }

    virtual const T* GetData() const
    {
        return &_data;
    }

    virtual T* GetData()
    {
        return &_data;
    }

    virtual void Copy(const T& other)
    {
        _data = other;
        _UpdateDataInSpec();
    }

    virtual void Set(const key_type& key, const mapped_type& other)
    {
        _data[key] = other;
        _UpdateDataInSpec();
    }

    virtual std::pair<iterator, bool> Insert(const value_type& value)
    {
        const std::pair<iterator, bool> insertStatus = _data.insert(value);
        if (insertStatus.second) {
            _UpdateDataInSpec();
        }

        return insertStatus;
    }

    virtual bool Erase(const key_type& key)
    {
        const bool didErase = (_data.erase(key) != 0);
        if (didErase) {
            _UpdateDataInSpec();
        }

        return didErase;
    }

    virtual SdfAllowed IsValidKey(const key_type& key) const
    {
        if (const SdfSchema::FieldDefinition* def = 
                _owner->GetSchema().GetFieldDefinition(_field)) {
            return def->IsValidMapKey(key);
        }
        return true;
    }

    virtual SdfAllowed IsValidValue(const mapped_type& value) const
    {
        if (const SdfSchema::FieldDefinition* def = 
                _owner->GetSchema().GetFieldDefinition(_field)) {
            return def->IsValidMapValue(value);
        }
        return true;
    }

private:
    void _UpdateDataInSpec()
    {
        TfAutoMallocTag2 tag("Sdf", "Sdf_LsdMapEditor::_UpdateDataInSpec");

        if (TF_VERIFY(_owner)) {
            if (_data.empty()) {
                _owner->ClearField(_field);
            }
            else {
                _owner->SetField(_field, _data);
            }
        }
    }

private:
    SdfSpecHandle _owner;
    TfToken _field;
    T _data;
};

//
// Factory functions
//

template <class T>
std::unique_ptr<Sdf_MapEditor<T> > 
Sdf_CreateMapEditor(const SdfSpecHandle& owner, const TfToken& field)
{
    return std::make_unique<Sdf_LsdMapEditor<T>>(owner, field);
}

//
// Template instantiations
//

#define SDF_INSTANTIATE_MAP_EDITOR(MapType)                          \
    template class Sdf_MapEditor<MapType>;                           \
    template class Sdf_LsdMapEditor<MapType>;                        \
    template std::unique_ptr<Sdf_MapEditor<MapType> >                \
        Sdf_CreateMapEditor(const SdfSpecHandle&, const TfToken&);   \

#include <pxr/vt/dictionary.h>
#include "./types.h"
SDF_INSTANTIATE_MAP_EDITOR(VtDictionary); 
SDF_INSTANTIATE_MAP_EDITOR(SdfVariantSelectionMap); 
SDF_INSTANTIATE_MAP_EDITOR(SdfRelocatesMap); 

}  // namespace pxr
