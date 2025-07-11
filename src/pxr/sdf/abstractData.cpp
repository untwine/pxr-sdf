// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./abstractData.h"
#include <pxr/trace/trace.h>

#include <cmath>
#include <ostream>
#include <vector>
#include <utility>

using std::vector;
using std::pair;
using std::make_pair;

namespace pxr {

TF_DEFINE_PUBLIC_TOKENS(SdfDataTokens, SDF_DATA_TOKENS);

////////////////////////////////////////////////////////////

SdfAbstractData::~SdfAbstractData()
{

}

struct SdfAbstractData_IsEmptyChecker : public SdfAbstractDataSpecVisitor
{
    SdfAbstractData_IsEmptyChecker() : isEmpty(true) { }
    virtual bool VisitSpec(const SdfAbstractData &, const SdfPath &)
    {
        isEmpty = false;
        return false;
    }

    virtual void Done(const SdfAbstractData&)
    {
        // Do nothing
    }

    bool isEmpty;
};

bool
SdfAbstractData::IsEmpty() const
{
    SdfAbstractData_IsEmptyChecker checker;
    VisitSpecs(&checker);
    return checker.isEmpty;
}

bool
SdfAbstractData::IsDetached() const
{
    return !StreamsData();
}

struct SdfAbstractData_CopySpecs : public SdfAbstractDataSpecVisitor
{
    SdfAbstractData_CopySpecs(SdfAbstractData* dest_) : dest(dest_) { }

    virtual bool VisitSpec(
        const SdfAbstractData& src, const SdfPath &path)
    {
        const std::vector<TfToken> keys = src.List(path);

        dest->CreateSpec(path, src.GetSpecType(path));
        TF_FOR_ALL(keyIt, keys) {
            dest->Set(path, *keyIt, src.Get(path, *keyIt));
        }
        return true;
    }

    virtual void Done(const SdfAbstractData&)
    {
        // Do nothing
    }

    SdfAbstractData* dest;
};

void
SdfAbstractData::CopyFrom(const SdfAbstractDataConstPtr& source)
{
    SdfAbstractData_CopySpecs copySpecsToThis(this);
    source->VisitSpecs(&copySpecsToThis);
}

// Visitor that checks whether all specs in the visited SdfAbstractData object
// exist in another SdfAbstractData object.
struct SdfAbstractData_CheckAllSpecsExist : public SdfAbstractDataSpecVisitor
{
    SdfAbstractData_CheckAllSpecsExist(const SdfAbstractData& data) 
        : passed(true), _data(data) { }

    virtual bool VisitSpec(
        const SdfAbstractData&, const SdfPath &path)
    {
        if (!_data.HasSpec(path)) {
            passed = false;
        }
        return passed;
    }

    virtual void Done(const SdfAbstractData&)
    {
        // Do nothing
    }

    bool passed;

private:
    const SdfAbstractData& _data;
};

// Visitor that checks whether all specs in the visited SdfAbstractData object
// have the same fields and contents as another SdfAbstractData object.
struct SdfAbstractData_CheckAllSpecsMatch : public SdfAbstractDataSpecVisitor
{
    SdfAbstractData_CheckAllSpecsMatch(const SdfAbstractData& rhs) 
        : passed(true), _rhs(rhs) { }

    virtual bool VisitSpec(
        const SdfAbstractData& lhs, const SdfPath &path)
    {
        return (passed = _AreSpecsAtPathEqual(lhs, _rhs, path));
    }

    virtual void Done(const SdfAbstractData&)
    {
        // Do nothing
    }

    bool passed;

private:
    static bool _AreSpecsAtPathEqual(
        const SdfAbstractData& lhs, const SdfAbstractData& rhs, 
        const SdfPath &path)
    {
        const TfTokenVector lhsFields = lhs.List(path);
        const TfTokenVector rhsFields = rhs.List(path);
        std::set<TfToken> lhsFieldSet( lhsFields.begin(), lhsFields.end() );
        std::set<TfToken> rhsFieldSet( rhsFields.begin(), rhsFields.end() );

        if (lhs.GetSpecType(path) != rhs.GetSpecType(path))
            return false;
        if (lhsFieldSet != rhsFieldSet)
            return false;

        TF_FOR_ALL(field, lhsFields) {
            // Note: this comparison forces manufacturing of VtValues.
            if (lhs.Get(path, *field) != rhs.Get(path, *field))
                return false;
        }

        return true;
    }

private:
    const SdfAbstractData& _rhs;
};

bool 
SdfAbstractData::Equals(const SdfAbstractDataRefPtr &rhs) const
{
    TRACE_FUNCTION();

    // Check that the set of specs matches.
    SdfAbstractData_CheckAllSpecsExist 
        rhsHasAllSpecsInThis(*get_pointer(rhs));
    VisitSpecs(&rhsHasAllSpecsInThis);
    if (!rhsHasAllSpecsInThis.passed)
        return false;

    SdfAbstractData_CheckAllSpecsExist thisHasAllSpecsInRhs(*this);
    rhs->VisitSpecs(&thisHasAllSpecsInRhs);
    if (!thisHasAllSpecsInRhs.passed)
        return false;

    // Check that every spec matches.
    SdfAbstractData_CheckAllSpecsMatch 
        thisSpecsMatchRhsSpecs(*get_pointer(rhs));
    VisitSpecs(&thisSpecsMatchRhsSpecs);
    return thisSpecsMatchRhsSpecs.passed;
}

// Visitor for collecting a sorted set of all paths in an SdfAbstractData.
struct SdfAbstractData_SortedPathCollector : public SdfAbstractDataSpecVisitor
{
    virtual bool VisitSpec(
        const SdfAbstractData& data, const SdfPath &path) 
    { 
        paths.insert(path);
        return true; 
    }

    virtual void Done(const SdfAbstractData&)
    {
        // Do nothing
    }

    SdfPathSet paths;
};

void 
SdfAbstractData::WriteToStream(std::ostream& os) const
{
    TRACE_FUNCTION();

    // We sort keys and fields below to ensure a stable output ordering.
    SdfAbstractData_SortedPathCollector collector;
    VisitSpecs(&collector);

    for (SdfPath const &path: collector.paths) {
        const SdfSpecType specType = GetSpecType(path);

        os << path << " " << TfEnum::GetDisplayName(specType) << '\n';

        const TfTokenVector fields = List(path);
        const std::set<TfToken> fieldSet(fields.begin(), fields.end());

        for (TfToken const &fieldName: fieldSet) {
            const VtValue value = Get(path, fieldName);
            os << "    " 
                 << fieldName << " "
                 << value.GetTypeName() << " " 
                 << value << '\n';
        }
    }
}

void
SdfAbstractData::VisitSpecs(SdfAbstractDataSpecVisitor* visitor) const
{
    if (TF_VERIFY(visitor)) {
        _VisitSpecs(visitor);
        visitor->Done(*this);
    }
}

bool
SdfAbstractData::HasSpecAndField(
    const SdfPath &path, const TfToken &fieldName,
    SdfAbstractDataValue *value, SdfSpecType *specType) const
{
    *specType = GetSpecType(path);
    return *specType != SdfSpecTypeUnknown && Has(path, fieldName, value);
}

bool
SdfAbstractData::HasSpecAndField(
    const SdfPath &path, const TfToken &fieldName,
    VtValue *value, SdfSpecType *specType) const
{
    *specType = GetSpecType(path);
    return *specType != SdfSpecTypeUnknown && Has(path, fieldName, value);
}

std::type_info const &
SdfAbstractData::GetTypeid(const SdfPath &path, const TfToken &fieldName) const
{
    return Get(path, fieldName).GetTypeid();
}

bool
SdfAbstractData::GetPreviousTimeSampleForPath(
    const SdfPath &path, double time, double* tPrevious) const
{
    double lower, upper;
    bool result = GetBracketingTimeSamplesForPath(path, time, &lower, &upper);
    if (result) {
        if (time < lower) {
            return false;
        }
        if (time == lower) {
            // time falls on a time sample and hence lower == time (and upper).
            // Go backwards to find the previous time sample using the 
            // bracketing time samples.
            double prevTime = 
                nexttoward(time, -std::numeric_limits<double>::infinity());
            result = GetBracketingTimeSamplesForPath(path, prevTime, &lower, 
                                                     &upper);
            if (!result || time == lower) {
                // couldn't get bracketing times again, or,
                // time is still same as lower, is only possible when time is 
                // same as the first time sample, we cannot determine the
                // previous time sample in this case.
                return false;
            }
        }
        *tPrevious = lower;
    }
    return result;
}

bool
SdfAbstractData::HasDictKey(const SdfPath &path,
                            const TfToken &fieldName,
                            const TfToken &keyPath,
                            SdfAbstractDataValue* value) const
{
    VtValue tmp;
    bool result = HasDictKey(path, fieldName, keyPath, value ? &tmp : NULL);
    if (result && value) {
        result = value->StoreValue(tmp);
    }
    return result;
}

bool
SdfAbstractData::HasDictKey(const SdfPath &path,
                            const TfToken &fieldName,
                            const TfToken &keyPath,
                            VtValue *value) const
{
    // Attempt to look up field.
    VtValue dictVal;
    if (Has(path, fieldName, &dictVal) && dictVal.IsHolding<VtDictionary>()) {
        // It's a dictionary -- attempt to find element at keyPath.
        if (VtValue const *v =
            dictVal.UncheckedGet<VtDictionary>().GetValueAtPath(keyPath)) {
            if (value)
                *value = *v;
            return true;
        }
    }
    return false;
}

VtValue
SdfAbstractData::GetDictValueByKey(const SdfPath &path,
                                   const TfToken &fieldName,
                                   const TfToken &keyPath) const
{
    VtValue result;
    HasDictKey(path, fieldName, keyPath, &result);
    return result;
}

void
SdfAbstractData::SetDictValueByKey(const SdfPath &path,
                                   const TfToken &fieldName,
                                   const TfToken &keyPath,
                                   const VtValue &value)
{
    if (value.IsEmpty()) {
        EraseDictValueByKey(path, fieldName, keyPath);
        return;
    }

    VtValue dictVal = Get(path, fieldName);

    // Swap out existing dictionary (if present).
    VtDictionary dict;
    dictVal.Swap(dict);

    // Now modify dict.
    dict.SetValueAtPath(keyPath, value);

    // Swap it back into the VtValue, and set it.
    dictVal.Swap(dict);
    Set(path, fieldName, dictVal);
}

void
SdfAbstractData::SetDictValueByKey(const SdfPath &path,
                                   const TfToken &fieldName,
                                   const TfToken &keyPath,
                                   const SdfAbstractDataConstValue& value)
{
    VtValue vtval;
    value.GetValue(&vtval);
    SetDictValueByKey(path, fieldName, keyPath, vtval);
}

void
SdfAbstractData::EraseDictValueByKey(const SdfPath &path,
                                     const TfToken &fieldName,
                                     const TfToken &keyPath)
{
    VtValue dictVal = Get(path, fieldName);

    if (dictVal.IsHolding<VtDictionary>()) {
        // Swap out existing dictionary (if present).
        VtDictionary dict;
        dictVal.Swap(dict);

        // Now modify dict.
        dict.EraseValueAtPath(keyPath);

        // Swap it back into the VtValue, and set it.
        if (dict.empty()) {
            Erase(path, fieldName);
        } else {
            dictVal.Swap(dict);
            Set(path, fieldName, dictVal);
        }
    }
}

std::vector<TfToken>
SdfAbstractData::ListDictKeys(const SdfPath &path,
                              const TfToken &fieldName,
                              const TfToken &keyPath) const
{
    vector<TfToken> result;
    VtValue dictVal = GetDictValueByKey(path, fieldName, keyPath);
    if (dictVal.IsHolding<VtDictionary>()) {
        VtDictionary const &dict = dictVal.UncheckedGet<VtDictionary>();
        result.reserve(dict.size());
        TF_FOR_ALL(i, dict)
            result.push_back(TfToken(i->first));
    }
    return result;
}


////////////////////////////////////////////////////////////

SdfAbstractDataSpecVisitor::~SdfAbstractDataSpecVisitor()
{
}

}  // namespace pxr
