// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_DATA_H
#define PXR_SDF_DATA_H

#include "./api.h"
#include "./abstractData.h"
#include "./path.h"
#include <pxr/tf/declarePtrs.h>
#include <pxr/tf/hashmap.h>
#include <pxr/tf/token.h>
#include <pxr/vt/value.h>

#include <vector>

namespace pxr {

TF_DECLARE_WEAK_AND_REF_PTRS(SdfData);

/// \class SdfData
///
/// SdfData provides concrete scene description data storage.
///
/// An SdfData is an implementation of SdfAbstractData that simply
/// stores specs and fields in a map keyed by path.
///
class SdfData : public SdfAbstractData
{
public:
    SdfData() {}
    SDF_API
    virtual ~SdfData();

    /// SdfAbstractData overrides

    SDF_API
    virtual bool StreamsData() const;

    SDF_API
    virtual bool IsDetached() const;

    SDF_API
    virtual void CreateSpec(const SdfPath& path, 
                            SdfSpecType specType);
    SDF_API
    virtual bool HasSpec(const SdfPath& path) const;
    SDF_API
    virtual void EraseSpec(const SdfPath& path);
    SDF_API
    virtual void MoveSpec(const SdfPath& oldPath, 
                          const SdfPath& newPath);
    SDF_API
    virtual SdfSpecType GetSpecType(const SdfPath& path) const;

    SDF_API
    virtual bool Has(const SdfPath& path, const TfToken &fieldName,
                     SdfAbstractDataValue* value) const;
    SDF_API
    virtual bool Has(const SdfPath& path, const TfToken& fieldName,
                     VtValue *value = NULL) const;
    SDF_API
    virtual bool
    HasSpecAndField(const SdfPath &path, const TfToken &fieldName,
                    SdfAbstractDataValue *value, SdfSpecType *specType) const;

    SDF_API
    virtual bool
    HasSpecAndField(const SdfPath &path, const TfToken &fieldName,
                    VtValue *value, SdfSpecType *specType) const;

    SDF_API
    virtual VtValue Get(const SdfPath& path, 
                        const TfToken& fieldName) const;
    SDF_API
    virtual void Set(const SdfPath& path, const TfToken& fieldName,
                     const VtValue & value);
    SDF_API
    virtual void Set(const SdfPath& path, const TfToken& fieldName,
                     const SdfAbstractDataConstValue& value);
    SDF_API
    virtual void Erase(const SdfPath& path, 
                       const TfToken& fieldName);
    SDF_API
    virtual std::vector<TfToken> List(const SdfPath& path) const;

    SDF_API
    virtual std::set<double>
    ListAllTimeSamples() const;
    
    SDF_API
    virtual std::set<double>
    ListTimeSamplesForPath(const SdfPath& path) const;

    SDF_API
    virtual bool
    GetBracketingTimeSamples(double time, double* tLower, double* tUpper) const;

    SDF_API
    virtual size_t
    GetNumTimeSamplesForPath(const SdfPath& path) const;

    SDF_API
    virtual bool
    GetBracketingTimeSamplesForPath(const SdfPath& path, 
                                    double time,
                                    double* tLower, double* tUpper) const;

    SDF_API
    virtual bool
    GetPreviousTimeSampleForPath(const SdfPath& path, double time, 
                                 double* tPrevious) const;

    SDF_API
    virtual bool
    QueryTimeSample(const SdfPath& path, double time,
                    SdfAbstractDataValue *optionalValue) const;
    SDF_API
    virtual bool
    QueryTimeSample(const SdfPath& path, double time, 
                    VtValue *value) const;

    SDF_API
    virtual void
    SetTimeSample(const SdfPath& path, double time, 
                  const VtValue & value);

    SDF_API
    virtual void
    EraseTimeSample(const SdfPath& path, double time);

protected:
    // SdfAbstractData overrides
    SDF_API
    virtual void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const;

private:
    const VtValue* _GetSpecTypeAndFieldValue(const SdfPath& path,
                                             const TfToken& field,
                                             SdfSpecType* specType) const;

    const VtValue* _GetFieldValue(const SdfPath& path,
                                  const TfToken& field) const;

    VtValue* _GetMutableFieldValue(const SdfPath& path,
                                   const TfToken& field);

    VtValue* _GetOrCreateFieldValue(const SdfPath& path,
                                    const TfToken& field);

private:
    // Backing storage for a single "spec" -- prim, property, etc.
    typedef std::pair<TfToken, VtValue> _FieldValuePair;
    struct _SpecData {
        _SpecData() : specType(SdfSpecTypeUnknown) {}
        
        SdfSpecType specType;
        std::vector<_FieldValuePair> fields;
    };

    // Hashtable storing _SpecData.
    typedef SdfPath _Key;
    typedef SdfPath::Hash _KeyHash;
    typedef TfHashMap<_Key, _SpecData, _KeyHash> _HashTable;

    _HashTable _data;
};

}  // namespace pxr

#endif // PXR_SDF_DATA_H
