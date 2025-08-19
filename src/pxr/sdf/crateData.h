// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_CRATE_DATA_H
#define PXR_SDF_CRATE_DATA_H

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/abstractData.h"

#include <pxr/tf/declarePtrs.h>
#include <pxr/tf/token.h>
#include <pxr/vt/value.h>

#include <memory>
#include <vector>
#include <set>

SDF_NAMESPACE_OPEN_SCOPE

class ArAsset;

/// \class Sdf_CrateData
///
class Sdf_CrateData : public SdfAbstractData
{
public:

    explicit Sdf_CrateData(bool detached);
    virtual ~Sdf_CrateData(); 

    static TfToken const &GetSoftwareVersionToken();

    static bool CanRead(const std::string &assetPath);
    static bool CanRead(const std::string &assetPath,
                        const std::shared_ptr<ArAsset> &asset);

    bool Save(const std::string &fileName);

    bool Export(const std::string &fileName);

    bool Open(const std::string &assetPath,
              bool detached);
    bool Open(const std::string &assetPath, 
              const std::shared_ptr<ArAsset> &asset,
              bool detached);

    virtual bool StreamsData() const;
    virtual void CreateSpec(const SdfPath &path, 
                            SdfSpecType specType);
    virtual bool HasSpec(const SdfPath &path) const;
    virtual void EraseSpec(const SdfPath &path);
    virtual void MoveSpec(const SdfPath& oldPath, 
                          const SdfPath& newPath);
    virtual SdfSpecType GetSpecType(const SdfPath &path) const;

    virtual bool Has(const SdfPath& path, const TfToken& fieldName,
                     SdfAbstractDataValue* value) const;
    virtual bool Has(const SdfPath& path, const TfToken& fieldName,
                     VtValue *value=nullptr) const;
    virtual bool
    HasSpecAndField(const SdfPath &path, const TfToken &fieldName,
                    SdfAbstractDataValue *value, SdfSpecType *specType) const;
    virtual bool
    HasSpecAndField(const SdfPath &path, const TfToken &fieldName,
                    VtValue *value, SdfSpecType *specType) const;

    virtual VtValue Get(const SdfPath& path, 
                        const TfToken& fieldName) const;
    virtual std::type_info const &GetTypeid(const SdfPath& path,
                                            const TfToken& fieldname) const;
    virtual void Set(const SdfPath& path, const TfToken& fieldName,
                     const VtValue& value);
    virtual void Set(const SdfPath& path, const TfToken& fieldName,
                     const SdfAbstractDataConstValue& value);
    virtual void Erase(const SdfPath& path, 
                       const TfToken& fieldName);
    virtual std::vector<TfToken> List(const SdfPath& path) const;
    
    /// \name Time-sample API
    /// @{

    virtual std::set<double>
    ListAllTimeSamples() const;
    
    virtual std::set<double>
    ListTimeSamplesForPath(const SdfPath& path) const;

    virtual bool
    GetBracketingTimeSamples(double time, double* tLower, double* tUpper) const;

    virtual size_t
    GetNumTimeSamplesForPath(const SdfPath& path) const;

    virtual bool
    GetBracketingTimeSamplesForPath(const SdfPath& path,
                                    double time,
                                    double* tLower, double* tUpper) const;

    virtual bool
    GetPreviousTimeSampleForPath(const SdfPath& path, double time,
                                 double* tPrevious) const;
    
    virtual bool
    QueryTimeSample(const SdfPath& path, double time,
                    SdfAbstractDataValue *value) const;
    virtual bool
    QueryTimeSample(const SdfPath& path, double time, 
                    VtValue *value) const;

    virtual void
    SetTimeSample(const SdfPath& path, double time, 
                  const VtValue & value);

    virtual void
    EraseTimeSample(const SdfPath& path, double time);

    /// @}

private:

    // SdfAbstractData overrides
    virtual void _VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const;

    friend class Sdf_CrateDataImpl;
    std::unique_ptr<class Sdf_CrateDataImpl> _impl;
};


SDF_NAMESPACE_CLOSE_SCOPE

#endif // PXR_SDF_CRATE_DATA_H
