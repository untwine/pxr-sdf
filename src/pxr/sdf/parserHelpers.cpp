// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include "./opaqueValue.h"
#include "./parserHelpers.h"
#include "./schema.h"
#include <pxr/gf/half.h>
#include <pxr/gf/matrix2d.h>
#include <pxr/gf/matrix3d.h>
#include <pxr/gf/matrix4d.h>
#include <pxr/gf/quatd.h>
#include <pxr/gf/quatf.h>
#include <pxr/gf/quath.h>
#include <pxr/gf/vec2d.h>
#include <pxr/gf/vec2f.h>
#include <pxr/gf/vec2h.h>
#include <pxr/gf/vec2i.h>
#include <pxr/gf/vec3d.h>
#include <pxr/gf/vec3f.h>
#include <pxr/gf/vec3h.h>
#include <pxr/gf/vec3i.h>
#include <pxr/gf/vec4d.h>
#include <pxr/gf/vec4f.h>
#include <pxr/gf/vec4h.h>
#include <pxr/gf/vec4i.h>
#include <pxr/tf/iterator.h>
#include <pxr/tf/stringUtils.h>
#include <pxr/plug/registry.h>
#include <pxr/vt/array.h>
#include <pxr/vt/value.h>

#include <algorithm>
#include <array>
#include <map>

namespace pxr {

namespace Sdf_ParserHelpers {

using std::string;
using std::vector;

// Check that there are enough values to parse so we don't overflow
#define CHECK_BOUNDS(count, name)                                          \
    if (index + count > vars.size()) {                                     \
        TF_CODING_ERROR("Not enough values to parse value of type %s",     \
                        name);                                             \
        throw std::bad_variant_access();                                            \
    }

inline void
MakeScalarValueImpl(string *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "string");
    *out = vars[index++].Get<std::string>();
}

inline void
MakeScalarValueImpl(TfToken *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "token");
    *out = TfToken(vars[index++].Get<std::string>());
}

inline void
MakeScalarValueImpl(double *out,
                                vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "double");
    *out = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(float *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "float");
    *out = vars[index++].Get<float>();
}

inline void
MakeScalarValueImpl(GfHalf *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "half");
    *out = GfHalf(vars[index++].Get<float>());
}

inline void
MakeScalarValueImpl(
    SdfTimeCode *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "timecode");
    *out = SdfTimeCode(vars[index++].Get<double>());
}

template <class Int>
inline std::enable_if_t<std::is_integral<Int>::value>
MakeScalarValueImpl(Int *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, ArchGetDemangled<Int>().c_str());
    *out = vars[index++].Get<Int>();
}

inline void
MakeScalarValueImpl(GfVec2d *out,
                                vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(2, "Vec2d");
    (*out)[0] = vars[index++].Get<double>();
    (*out)[1] = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(GfVec2f *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(2, "Vec2f");
    (*out)[0] = vars[index++].Get<float>();
    (*out)[1] = vars[index++].Get<float>();
}

inline void
MakeScalarValueImpl(GfVec2h *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(2, "Vec2h");
    (*out)[0] = GfHalf(vars[index++].Get<float>());
    (*out)[1] = GfHalf(vars[index++].Get<float>());
}

inline void
MakeScalarValueImpl(GfVec2i *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(2, "Vec2i");
    (*out)[0] = vars[index++].Get<int>();
    (*out)[1] = vars[index++].Get<int>();
}

inline void
MakeScalarValueImpl(GfVec3d *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(3, "Vec3d");
    (*out)[0] = vars[index++].Get<double>();
    (*out)[1] = vars[index++].Get<double>();
    (*out)[2] = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(GfVec3f *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(3, "Vec3f");
    (*out)[0] = vars[index++].Get<float>();
    (*out)[1] = vars[index++].Get<float>();
    (*out)[2] = vars[index++].Get<float>();
}

inline void
MakeScalarValueImpl(GfVec3h *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(3, "Vec3h");
    (*out)[0] = GfHalf(vars[index++].Get<float>());
    (*out)[1] = GfHalf(vars[index++].Get<float>());
    (*out)[2] = GfHalf(vars[index++].Get<float>());
}

inline void
MakeScalarValueImpl(GfVec3i *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(3, "Vec3i");
    (*out)[0] = vars[index++].Get<int>();
    (*out)[1] = vars[index++].Get<int>();
    (*out)[2] = vars[index++].Get<int>();
}

inline void
MakeScalarValueImpl(GfVec4d *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Vec4d");
    (*out)[0] = vars[index++].Get<double>();
    (*out)[1] = vars[index++].Get<double>();
    (*out)[2] = vars[index++].Get<double>();
    (*out)[3] = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(GfVec4f *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Vec4f");
    (*out)[0] = vars[index++].Get<float>();
    (*out)[1] = vars[index++].Get<float>();
    (*out)[2] = vars[index++].Get<float>();
    (*out)[3] = vars[index++].Get<float>();
}

inline void
MakeScalarValueImpl(GfVec4h *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Vec4h");
    (*out)[0] = GfHalf(vars[index++].Get<float>());
    (*out)[1] = GfHalf(vars[index++].Get<float>());
    (*out)[2] = GfHalf(vars[index++].Get<float>());
    (*out)[3] = GfHalf(vars[index++].Get<float>());
}

inline void
MakeScalarValueImpl(GfVec4i *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Vec4i");
    (*out)[0] = vars[index++].Get<int>();
    (*out)[1] = vars[index++].Get<int>();
    (*out)[2] = vars[index++].Get<int>();
    (*out)[3] = vars[index++].Get<int>();
}

inline void
MakeScalarValueImpl(GfMatrix2d *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Matrix2d");
    (*out)[0][0] = vars[index++].Get<double>();
    (*out)[0][1] = vars[index++].Get<double>();
    (*out)[1][0] = vars[index++].Get<double>();
    (*out)[1][1] = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(GfMatrix3d *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(9, "Matrix3d");
    (*out)[0][0] = vars[index++].Get<double>();
    (*out)[0][1] = vars[index++].Get<double>();
    (*out)[0][2] = vars[index++].Get<double>();
    (*out)[1][0] = vars[index++].Get<double>();
    (*out)[1][1] = vars[index++].Get<double>();
    (*out)[1][2] = vars[index++].Get<double>();
    (*out)[2][0] = vars[index++].Get<double>();
    (*out)[2][1] = vars[index++].Get<double>();
    (*out)[2][2] = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(GfMatrix4d *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(16, "Matrix4d");
    (*out)[0][0] = vars[index++].Get<double>();
    (*out)[0][1] = vars[index++].Get<double>();
    (*out)[0][2] = vars[index++].Get<double>();
    (*out)[0][3] = vars[index++].Get<double>();
    (*out)[1][0] = vars[index++].Get<double>();
    (*out)[1][1] = vars[index++].Get<double>();
    (*out)[1][2] = vars[index++].Get<double>();
    (*out)[1][3] = vars[index++].Get<double>();
    (*out)[2][0] = vars[index++].Get<double>();
    (*out)[2][1] = vars[index++].Get<double>();
    (*out)[2][2] = vars[index++].Get<double>();
    (*out)[2][3] = vars[index++].Get<double>();
    (*out)[3][0] = vars[index++].Get<double>();
    (*out)[3][1] = vars[index++].Get<double>();
    (*out)[3][2] = vars[index++].Get<double>();
    (*out)[3][3] = vars[index++].Get<double>();
}

inline void
MakeScalarValueImpl(GfQuatd *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Quatd");
    // Values in order are re, i, j, k.
    GfVec3d imag; double re;
    MakeScalarValueImpl(&re, vars, index);
    out->SetReal(re);
    MakeScalarValueImpl(&imag, vars, index);
    out->SetImaginary(imag);
}

inline void
MakeScalarValueImpl(GfQuatf *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Quatf");
    // Values in order are re, i, j, k.
    GfVec3f imag; float re;
    MakeScalarValueImpl(&re, vars, index);
    out->SetReal(re);
    MakeScalarValueImpl(&imag, vars, index);
    out->SetImaginary(imag);
}

inline void
MakeScalarValueImpl(GfQuath *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(4, "Quath");
    // Values in order are re, i, j, k.
    GfVec3h imag; GfHalf re;
    MakeScalarValueImpl(&re, vars, index);
    out->SetReal(re);
    MakeScalarValueImpl(&imag, vars, index);
    out->SetImaginary(imag);
}

inline void
MakeScalarValueImpl(
    SdfAssetPath *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "asset");
    *out = vars[index++].Get<SdfAssetPath>();
}

inline void
MakeScalarValueImpl(
    SdfPathExpression *out, vector<Value> const &vars, size_t &index) {
    CHECK_BOUNDS(1, "pathExpression");
    *out = SdfPathExpression(vars[index++].Get<std::string>());
}

inline void
MakeScalarValueImpl(
    SdfOpaqueValue *out, vector<Value> const &vars, size_t &index) {
    TF_CODING_ERROR("Found authored opinion for opaque attribute");
    throw std::bad_variant_access();
}

template <typename T>
inline VtValue
MakeScalarValueTemplate(vector<unsigned int> const &,
                        vector<Value> const &vars, size_t &index,
                        string *errStrPtr) {
    T t;
    size_t origIndex = index;
    try {
        MakeScalarValueImpl(&t, vars, index);
    } catch (const std::bad_variant_access &) {
        *errStrPtr = TfStringPrintf("Failed to parse value (at sub-part %zd "
                                    "if there are multiple parts)",
                                    (index - origIndex) - 1);
        return VtValue();
    }
    return VtValue(t);
}

template <typename T>
inline VtValue
MakeShapedValueTemplate(vector<unsigned int> const &shape,
                        vector<Value> const &vars, size_t &index,
                        string *errStrPtr) {
    if (shape.empty())
        return VtValue(VtArray<T>());
//    TF_AXIOM(shape.size() == 1);
    unsigned int size = 1;
    TF_FOR_ALL(i, shape)
        size *= *i;

    VtArray<T> array(size);
    size_t shapeIndex = 0;
    size_t origIndex = index;
    try {
        TF_FOR_ALL(i, array) {
            MakeScalarValueImpl(&(*i), vars, index);
            shapeIndex++;
        }
    } catch (const std::bad_variant_access &) {
        *errStrPtr = TfStringPrintf("Failed to parse at element %zd "
                                    "(at sub-part %zd if there are "
                                    "multiple parts)", shapeIndex,
                                    (index - origIndex) - 1);
        return VtValue();
    }
    return VtValue(array);
}

typedef std::map<std::string, ValueFactory> _ValueFactoryMap;

// Walk through types and register factories.
struct _MakeFactoryMap {

    explicit _MakeFactoryMap(_ValueFactoryMap *factories) :
        _factories(factories) {}

    template <class CppType>
    void add(const SdfValueTypeName& scalar, const char* alias = NULL)
    {
        static const bool isShaped = true;

        const SdfValueTypeName array = scalar.GetArrayType();

        const std::string scalarName =
            alias ? std::string(alias)        : scalar.GetAsToken().GetString();
        const std::string arrayName =
            alias ? std::string(alias) + "[]" : array.GetAsToken().GetString();

        _ValueFactoryMap &f = *_factories;
        f[scalarName] = ValueFactory(
            scalarName, scalar.GetDimensions(), !isShaped,
            MakeScalarValueTemplate<CppType>);
        f[arrayName] = ValueFactory(
            arrayName, array.GetDimensions(), isShaped,
            MakeShapedValueTemplate<CppType>);
    }
    
    _ValueFactoryMap *_factories;
};

TF_MAKE_STATIC_DATA(_ValueFactoryMap, _valueFactories) {
    _MakeFactoryMap builder(_valueFactories);
    // XXX: Would be better if SdfValueTypeName had a method to take
    //      a vector of VtValues and return a VtValue holding the
    //      appropriate C++ type (which mostly involves moving the
    //      MakeScalarValueImpl functions into the value type name
    //      registration code).  Then we could do this:
    //     for (const auto& typeName : SdfSchema::GetInstance().GetAllTypes()) {
    //        builder(typeName);
    //    }
    //            For symmetry (and I think it would actually be useful
    //            when converting usd into other formats) there should be
    //            a method to convert a VtValue holding the appropriate C++
    //            type into a vector of VtValues holding a primitive type.
    //            E.g. a VtValue holding a GfVec3f would return three
    //            VtValues each holding a float.
    builder.add<bool>(SdfValueTypeNames->Bool);
    builder.add<uint8_t>(SdfValueTypeNames->UChar);
    builder.add<int32_t>(SdfValueTypeNames->Int);
    builder.add<uint32_t>(SdfValueTypeNames->UInt);
    builder.add<int64_t>(SdfValueTypeNames->Int64);
    builder.add<uint64_t>(SdfValueTypeNames->UInt64);
    builder.add<GfHalf>(SdfValueTypeNames->Half);
    builder.add<float>(SdfValueTypeNames->Float);
    builder.add<double>(SdfValueTypeNames->Double);
    builder.add<SdfTimeCode>(SdfValueTypeNames->TimeCode);
    builder.add<std::string>(SdfValueTypeNames->String);
    builder.add<TfToken>(SdfValueTypeNames->Token);
    builder.add<SdfAssetPath>(SdfValueTypeNames->Asset);
    builder.add<SdfOpaqueValue>(SdfValueTypeNames->Opaque);
    builder.add<SdfOpaqueValue>(SdfValueTypeNames->Group);
    builder.add<SdfPathExpression>(SdfValueTypeNames->PathExpression);

    builder.add<GfVec2i>(SdfValueTypeNames->Int2);
    builder.add<GfVec2h>(SdfValueTypeNames->Half2);
    builder.add<GfVec2f>(SdfValueTypeNames->Float2);
    builder.add<GfVec2d>(SdfValueTypeNames->Double2);
    builder.add<GfVec3i>(SdfValueTypeNames->Int3);
    builder.add<GfVec3h>(SdfValueTypeNames->Half3);
    builder.add<GfVec3f>(SdfValueTypeNames->Float3);
    builder.add<GfVec3d>(SdfValueTypeNames->Double3);
    builder.add<GfVec4i>(SdfValueTypeNames->Int4);
    builder.add<GfVec4h>(SdfValueTypeNames->Half4);
    builder.add<GfVec4f>(SdfValueTypeNames->Float4);
    builder.add<GfVec4d>(SdfValueTypeNames->Double4);
    builder.add<GfVec3h>(SdfValueTypeNames->Point3h);
    builder.add<GfVec3f>(SdfValueTypeNames->Point3f);
    builder.add<GfVec3d>(SdfValueTypeNames->Point3d);
    builder.add<GfVec3h>(SdfValueTypeNames->Vector3h);
    builder.add<GfVec3f>(SdfValueTypeNames->Vector3f);
    builder.add<GfVec3d>(SdfValueTypeNames->Vector3d);
    builder.add<GfVec3h>(SdfValueTypeNames->Normal3h);
    builder.add<GfVec3f>(SdfValueTypeNames->Normal3f);
    builder.add<GfVec3d>(SdfValueTypeNames->Normal3d);
    builder.add<GfVec3h>(SdfValueTypeNames->Color3h);
    builder.add<GfVec3f>(SdfValueTypeNames->Color3f);
    builder.add<GfVec3d>(SdfValueTypeNames->Color3d);
    builder.add<GfVec4h>(SdfValueTypeNames->Color4h);
    builder.add<GfVec4f>(SdfValueTypeNames->Color4f);
    builder.add<GfVec4d>(SdfValueTypeNames->Color4d);
    builder.add<GfQuath>(SdfValueTypeNames->Quath);
    builder.add<GfQuatf>(SdfValueTypeNames->Quatf);
    builder.add<GfQuatd>(SdfValueTypeNames->Quatd);
    builder.add<GfMatrix2d>(SdfValueTypeNames->Matrix2d);
    builder.add<GfMatrix3d>(SdfValueTypeNames->Matrix3d);
    builder.add<GfMatrix4d>(SdfValueTypeNames->Matrix4d);
    builder.add<GfMatrix4d>(SdfValueTypeNames->Frame4d);
    builder.add<GfVec2f>(SdfValueTypeNames->TexCoord2f);
    builder.add<GfVec2d>(SdfValueTypeNames->TexCoord2d);
    builder.add<GfVec2h>(SdfValueTypeNames->TexCoord2h);
    builder.add<GfVec3f>(SdfValueTypeNames->TexCoord3f);
    builder.add<GfVec3d>(SdfValueTypeNames->TexCoord3d);
    builder.add<GfVec3h>(SdfValueTypeNames->TexCoord3h);

    // XXX: Backwards compatibility.  These should be removed when
    //      all assets are updated.  At the time of this writing
    //      under pxr only assets used by usdImaging need updating.
    //      Those assets must be moved anyway for open sourcing so
    //      I'm leaving this for now.  (Also note that at least one
    //      of those tests, testUsdImagingEmptyMesh, uses the prim
    //      type PxVolume which is not in pxr.)  Usd assets outside
    //      pxr must also be updated.
    builder.add<GfVec2i>(SdfValueTypeNames->Int2, "Vec2i");
    builder.add<GfVec2h>(SdfValueTypeNames->Half2, "Vec2h");
    builder.add<GfVec2f>(SdfValueTypeNames->Float2, "Vec2f");
    builder.add<GfVec2d>(SdfValueTypeNames->Double2, "Vec2d");
    builder.add<GfVec3i>(SdfValueTypeNames->Int3, "Vec3i");
    builder.add<GfVec3h>(SdfValueTypeNames->Half3, "Vec3h");
    builder.add<GfVec3f>(SdfValueTypeNames->Float3, "Vec3f");
    builder.add<GfVec3d>(SdfValueTypeNames->Double3, "Vec3d");
    builder.add<GfVec4i>(SdfValueTypeNames->Int4, "Vec4i");
    builder.add<GfVec4h>(SdfValueTypeNames->Half4, "Vec4h");
    builder.add<GfVec4f>(SdfValueTypeNames->Float4, "Vec4f");
    builder.add<GfVec4d>(SdfValueTypeNames->Double4, "Vec4d");
    builder.add<GfVec3f>(SdfValueTypeNames->Point3f, "PointFloat");
    builder.add<GfVec3d>(SdfValueTypeNames->Point3d, "Point");
    builder.add<GfVec3f>(SdfValueTypeNames->Vector3f, "NormalFloat");
    builder.add<GfVec3d>(SdfValueTypeNames->Vector3d, "Normal");
    builder.add<GfVec3f>(SdfValueTypeNames->Normal3f, "VectorFloat");
    builder.add<GfVec3d>(SdfValueTypeNames->Normal3d, "Vector");
    builder.add<GfVec3f>(SdfValueTypeNames->Color3f, "ColorFloat");
    builder.add<GfVec3d>(SdfValueTypeNames->Color3d, "Color");
    builder.add<GfQuath>(SdfValueTypeNames->Quath, "Quath");
    builder.add<GfQuatf>(SdfValueTypeNames->Quatf, "Quatf");
    builder.add<GfQuatd>(SdfValueTypeNames->Quatd, "Quatd");
    builder.add<GfMatrix2d>(SdfValueTypeNames->Matrix2d, "Matrix2d");
    builder.add<GfMatrix3d>(SdfValueTypeNames->Matrix3d, "Matrix3d");
    builder.add<GfMatrix4d>(SdfValueTypeNames->Matrix4d, "Matrix4d");
    builder.add<GfMatrix4d>(SdfValueTypeNames->Frame4d, "Frame");
    builder.add<GfMatrix4d>(SdfValueTypeNames->Matrix4d, "Transform");
    builder.add<int>(SdfValueTypeNames->Int, "PointIndex");
    builder.add<int>(SdfValueTypeNames->Int, "EdgeIndex");
    builder.add<int>(SdfValueTypeNames->Int, "FaceIndex");
    builder.add<TfToken>(SdfValueTypeNames->Token, "Schema");

    // Set up the special None factory.
    (*_valueFactories)[std::string("None")] = ValueFactory(
        std::string(""), SdfTupleDimensions(), false, NULL);

}

ValueFactory const &GetValueFactoryForMenvaName(std::string const &name,
                                                bool *found)
{
    _ValueFactoryMap::const_iterator it = _valueFactories->find(name);
    if (it != _valueFactories->end()) {
        *found = true;
        return it->second;
    }
    
    // No factory for given name.
    static ValueFactory const& none = (*_valueFactories)[std::string("None")];
    *found = false;
    return none;
}

} // namespace Sdf_ParserHelpers

bool
Sdf_BoolFromString( const std::string &str, bool *parseOk )
{
    if (parseOk)
        *parseOk = true;

    std::string s = str;
    std::transform(s.begin(), s.end(), s.begin(), ::tolower);

    if (strcmp(s.c_str(), "false") == 0)
        return false;
    if (strcmp(s.c_str(), "true") == 0)
        return true;
    if (strcmp(s.c_str(), "no") == 0)
        return false;
    if (strcmp(s.c_str(), "yes") == 0)
        return true;

    if (strcmp(s.c_str(), "0") == 0)
        return false;
    if (strcmp(s.c_str(), "1") == 0)
        return true;

    if (parseOk)
        *parseOk = false;
    return true;
}

std::string
Sdf_EvalQuotedString(const char* x, size_t n, size_t trimBothSides, 
                     unsigned int* numLines)
{
    std::string ret;

    // Handle empty strings
    if (n <= 2 * trimBothSides)
        return ret;

    n -= 2 * trimBothSides;

    // Use local buf, or malloc one if not enough space.
    // (this is a little too much if there are escape chars in the string,
    // but we can live with it to avoid traversing the string twice)
    static const size_t LocalSize = 2048;
    char localBuf[LocalSize];
    char *buf = n <= LocalSize ? localBuf : (char *)malloc(n);

    char *s = buf;

    const char *p = x + trimBothSides;
    const char * const end = x + trimBothSides + n;

    while (p < end) {
        const char *escOrEnd =
            static_cast<const char *>(memchr(p, '\\', std::distance(p, end)));
        if (!escOrEnd) {
            escOrEnd = end;
        }
               
        const size_t nchars = std::distance(p, escOrEnd);
        memcpy(s, p, nchars);
        s += nchars;
        p += nchars;

        if (escOrEnd != end) {
            TfEscapeStringReplaceChar(&p, &s);
            ++p;
        }
    }

    // Trim to final length.
    std::string(buf, s-buf).swap(ret);
    if (buf != localBuf) {
        free(buf);
    }

    if (numLines) {
        *numLines = std::count(ret.begin(), ret.end(), '\n');
    }
    
    return ret;
}

std::string 
Sdf_EvalAssetPath(const char* x, size_t n, bool tripleDelimited)
{
    // See _StringFromAssetPath for the code that writes asset paths.

    // Asset paths are assumed to only contain printable characters and 
    // no escape sequences except for the "@@@" delimiter.
    const int numDelimiters = tripleDelimited ? 3 : 1;
    std::string ret(x + numDelimiters, n - (2 * numDelimiters));
    if (tripleDelimited) {
        ret = TfStringReplace(ret, "\\@@@", "@@@");
    }

    // Go through SdfAssetPath for validation -- this will raise an error and
    // produce the empty asset path if 'ret' contains invalid characters.
    return SdfAssetPath(ret).GetAssetPath();
}

}  // namespace pxr
