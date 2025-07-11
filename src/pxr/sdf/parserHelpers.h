// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_PARSER_HELPERS_H
#define PXR_SDF_PARSER_HELPERS_H

#include "./assetPath.h"
#include "./valueTypeName.h"
#include <pxr/arch/inttypes.h>
#include <pxr/gf/numericCast.h>
#include <pxr/vt/value.h>

#include <functional>
#include <limits>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

namespace pxr {

bool Sdf_BoolFromString(const std::string &, bool *parseOk);

namespace Sdf_ParserHelpers {

// Internal variant type.
using _Variant = std::variant<uint64_t, int64_t, double,
                              std::string, TfToken, SdfAssetPath>;

////////////////////////////////////////////////////////////////////////
// Utilities that implement the Sdf_ParserHelpers::Value::Get<T>() method.  The
// _GetImpl<T> class template provides the right implementation for T.  There
// are several partial specializations that provide the right behavior for
// various types.

// General Get case, requires exact match.
template <class T, class Enable = void>
struct _GetImpl
{
    typedef const T &ResultType;
    static const T &Visit(_Variant const &variant) {
        return std::get<T>(variant);
    }
};

////////////////////////////////////////////////////////////////////////
// _GetImpl<T> for integral type T.  Convert finite doubles by static_cast,
// throw bad_variant_access for non-finite doubles.  Throw bad_variant_access
// for out-of-range integral values.
template <class T>
struct _GetImpl<
    T, std::enable_if_t<std::is_integral<T>::value>>
{
    typedef T ResultType;

    T Visit(_Variant const &variant) {
        return std::visit(*this, variant);
    }

    // Fallback case: throw bad_variant_access.
    template <class Held>
    T operator()(Held held) { throw std::bad_variant_access(); }

    // Attempt to cast unsigned and signed int64_t.
    T operator()(uint64_t in) { return _Cast(in); }
    T operator()(int64_t in) { return _Cast(in); }

    // Attempt to cast finite doubles, throw otherwise.
    T operator()(double in) {
        if (std::isfinite(in))
            return _Cast(in);
        throw std::bad_variant_access();
    }

private:
    template <class In>
    T _Cast(In in) {
        try {
            return GfNumericCast<T>(in).value();
        } catch (const std::bad_optional_access &) {
            throw std::bad_variant_access();
        }
    }
};
////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////
// _GetImpl<T> for floating point type T.  Attempts to cast numeric values.
// Also handles strings like "inf", "-inf", and "nan" to produce +/- infinity
// and a quiet NaN.
template <class T>
struct _GetImpl<
    T, std::enable_if_t<std::is_floating_point<T>::value>>
{
    typedef T ResultType;

    T Visit(_Variant const &variant) {
        return std::visit(*this, variant);
    }

    // Fallback case: throw bad_variant_access.
    template <class Held>
    T operator()(Held held) { throw std::bad_variant_access(); }

    // For numeric types, attempt to cast.
    T operator()(uint64_t in) { return _Cast(in); }
    T operator()(int64_t in) { return _Cast(in); }
    T operator()(double in) { return static_cast<T>(in); }
    
    // Convert special strings if possible.
    T operator()(const std::string &str) { return _FromString(str); }
    T operator()(const TfToken &tok) { return _FromString(tok.GetString()); }

private:
    T _FromString(const std::string &str) const {
        // Special case the strings 'inf', '-inf' and 'nan'.
        if (str == "inf") 
            return std::numeric_limits<T>::infinity();
        if (str == "-inf")
            return -std::numeric_limits<T>::infinity();
        if (str == "nan")
            return std::numeric_limits<T>::quiet_NaN();
        throw std::bad_variant_access();
    }

    template <class In>
    T _Cast(In in) {
        try {
            return GfNumericCast<T>(in).value();
        } catch (const std::bad_optional_access &) {
            throw std::bad_variant_access();
        }
    }
};


////////////////////////////////////////////////////////////////////////

// Get an asset path: converts string to asset path, otherwise throw
// bad_variant_access.
template <>
struct _GetImpl<SdfAssetPath>
{
    typedef SdfAssetPath ResultType;

    SdfAssetPath Visit(_Variant const &variant) {
        if (std::string const *str = std::get_if<std::string>(&variant))
            return SdfAssetPath(*str);
        return std::get<SdfAssetPath>(variant);
    }
};

// Get a bool.  Numbers are considered true if nonzero, false otherwise.
// Strings and tokens get parsed via Sdf_BoolFromString.  Otherwise throw
// bad_variant_access.
template <>
struct _GetImpl<bool>
{
    typedef bool ResultType;
    
    bool Visit(_Variant const &variant) {
        return std::visit(*this, variant);
    }

    // Parse string via Sdf_BoolFromString.
    bool operator()(const std::string &str) {
        bool parseOK = false;
        bool result = Sdf_BoolFromString(str, &parseOK);
        if (!parseOK)
            throw std::bad_variant_access();
        return result;
    }

    // Treat tokens as strings.
    bool operator()(const TfToken &tok) { return (*this)(tok.GetString()); }

    // For numbers, return true if not zero.
    template <class Number>
    std::enable_if_t<std::is_arithmetic<Number>::value, bool>
    operator()(Number val) {
        return val != static_cast<Number>(0);
    }

    // For anything else, throw bad_variant_access().
    template <class T>
    std::enable_if_t<!std::is_arithmetic<T>::value, bool>
    operator()(T) { throw std::bad_variant_access(); }
};

// A parser value.  This is used as the fundamental value object in the text
// parser.  It can hold one of a few different types: (u)int64_t, double,
// string, TfToken, and SdfAssetPath.  The lexer only ever produces Value
// objects holding (u)int64_t, double, and string.  The presence of TfToken and
// SdfAssetPath here are for a relatively obscure case where we're parsing a
// value whose type is unknown to the parser.  See
// StartRecordingString/StopRecordingString/IsRecordingString in
// parserValueContext.{cpp.h}.  We'd like to change this.
//
// Value's primary function is to provide a Get<T>() convenience API that
// handles appropriate conversions from the held types.  For example, it is okay
// to call Get<float>() on a Value that's holding an integral type, a double, or
// a string if that string's value is one of 'inf', '-inf', or 'nan'.  Similarly
// Get<bool>() works on numbers and strings like 'yes', 'no', 'on', 'off',
// 'true', 'false'.  If a Get<T>() call fails, it throws
// std::bad_variant_access, which the parser responds to and raises a parse
// error.
//
// The lexer constructs Value objects from input tokens.  It creates them to
// retain all the input information possible.  For example, negative integers
// are stored as int64_t Values, positive numbers are stored as uint64_t values,
// and so on.  As a special case of this, '-0' is stored as a double, since it
// is the only way to preserve a signed zero (integral types have no signed
// zero).
struct Value
{
    // Default constructor leaves the value in an undefined state.
    Value() {}
    
    // Construct and implicitly convert from an integral type \p Int.  If \p Int
    // is signed, the resulting value holds an 'int64_t' internally.  If \p Int
    // is unsigned, the result value holds an 'uint64_t'.
    template <class Int>
    Value(Int in, std::enable_if_t<std::is_integral<Int>::value> * = 0) {
        if (std::is_signed<Int>::value) {
            _variant = static_cast<int64_t>(in);
        } else {
            _variant = static_cast<uint64_t>(in);
        }
    }
    
    // Construct and implicitly convert from a floating point type \p Flt.  The
    // resulting value holds a double internally.
    template <class Flt>
    Value(Flt in, std::enable_if_t<std::is_floating_point<Flt>::value> * = 0) :
        _variant(static_cast<double>(in)) {}
    
    // Construct and implicitly convert from std::string.
    Value(const std::string &in) : _variant(in) {}

    // Construct and implicitly convert from TfToken.
    Value(const TfToken &in) : _variant(in) {}

    // Construct and implicitly convert from SdfAssetPath.
    Value(const SdfAssetPath &in) : _variant(in) {}
    
    // Attempt to get a value of type T from this Value, applying appropriate
    // conversions.  If this value cannot be converted to T, throw
    // std::bad_variant_access.
    template <class T>
    typename _GetImpl<T>::ResultType Get() const {
        return _GetImpl<T>().Visit(_variant);
    }

    // Hopefully short-lived API that applies an external visitor to the held
    // variant type.
    template <class Visitor>
    auto
    ApplyVisitor(const Visitor &visitor) {
        return std::visit(visitor, _variant);
    }

    template <class Visitor>
    auto
    ApplyVisitor(Visitor &visitor) {
        return std::visit(visitor, _variant);
    }

    template <class Visitor>
    auto
    ApplyVisitor(const Visitor &visitor) const {
        return std::visit(visitor, _variant);
    }

    template <class Visitor>
    auto
    ApplyVisitor(Visitor &visitor) const {
        return std::visit(visitor, _variant);
    }

private:
    _Variant _variant;
};

typedef std::function<VtValue (std::vector<unsigned int> const &,
                               std::vector<Value> const &,
                               size_t &, std::string *)> ValueFactoryFunc;

struct ValueFactory {
    ValueFactory() {}

    ValueFactory(std::string typeName_, SdfTupleDimensions dimensions_,
                 bool isShaped_, ValueFactoryFunc func_)
        : typeName(typeName_),
          dimensions(dimensions_),
          isShaped(isShaped_),
          func(func_) {}

    std::string typeName;
    SdfTupleDimensions dimensions;
    bool isShaped;
    ValueFactoryFunc func;
};

ValueFactory const &GetValueFactoryForMenvaName(std::string const &name,
                                                bool *found);
}

/// Converts a string to a bool.
/// Accepts case insensitive "yes", "no", "false", true", "0", "1".
/// Defaults to "true" if the string is not recognized.
///
/// If parseOK is supplied, the pointed-to bool will be set to indicate
/// whether the parse was successful.
bool Sdf_BoolFromString(const std::string &s, bool *parseOk = NULL);

// Read the quoted string at [x..x+n], trimming 'trimBothSides' number
// of chars from either side, and evaluating any embedded escaped characters.
// If numLines is given, it will be populated with the number of newline
// characters present in the original string.
std::string Sdf_EvalQuotedString(const char* x, size_t n, size_t trimBothSides,
                                 unsigned int* numLines = NULL);

// Read the string representing an asset path at [x..x+n]. If tripleDelimited
// is true, the string is assumed to have 3 delimiters on both sides of the
// asset path, otherwise the string is assumed to have just 1.
std::string Sdf_EvalAssetPath(const char* x, size_t n, bool tripleDelimited);

}  // namespace pxr

#endif // PXR_SDF_PARSER_HELPERS_H
