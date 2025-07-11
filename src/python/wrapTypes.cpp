// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#include <pxr/sdf/types.h>
#include <pxr/sdf/attributeSpec.h>
#include <pxr/sdf/listOp.h>
#include <pxr/sdf/primSpec.h>
#include <pxr/sdf/propertySpec.h>
#include <pxr/sdf/pyChildrenView.h>
#include <pxr/sdf/path.h>
#include <pxr/sdf/proxyTypes.h>
#include <pxr/sdf/pyListOp.h>
#include <pxr/sdf/pyListEditorProxy.h>
#include <pxr/sdf/pyListProxy.h>
#include <pxr/sdf/pyMapEditProxy.h>
#include <pxr/sdf/reference.h>
#include <pxr/sdf/relationshipSpec.h>
#include <pxr/sdf/variantSetSpec.h>
#include <pxr/sdf/variantSpec.h>

#include <pxr/tf/pyContainerConversions.h>
#include <pxr/tf/pyEnum.h>
#include <pxr/tf/pyStaticTokens.h>

#include <pxr/vt/valueFromPython.h>

#include <pxr/boost/python.hpp>

using std::string;

using namespace pxr;

using namespace pxr::boost::python;

namespace {

struct Sdf_TimeSampleMapConverter {
public:
    static PyObject* convert(SdfTimeSampleMap const &c)
    {
        pxr::boost::python::dict result = TfPyCopyMapToDictionary(c);
        return pxr::boost::python::incref(result.ptr());
    }
};

template <class MapType>
struct Sdf_MapTypeConverter {
public:
    using KeyType = typename MapType::key_type;
    using ValueType = typename MapType::mapped_type;

    Sdf_MapTypeConverter()
    {
        pxr::boost::python::converter::registry::push_back(
            &Sdf_MapTypeConverter::convertible,
            &Sdf_MapTypeConverter::construct,
            pxr::boost::python::type_id<MapType>());
        to_python_converter<MapType,
                            Sdf_MapTypeConverter>();
    }

    static void* convertible(PyObject* obj_ptr)
    {
        return _convert(obj_ptr, NULL);
    }

    static void construct(
      PyObject* obj_ptr,
      pxr::boost::python::converter::rvalue_from_python_stage1_data* data)
    {
        void* storage = (
            (converter::rvalue_from_python_storage<MapType>*)
            data)->storage.bytes;
        new (storage) MapType();
        data->convertible = storage;
        _convert(obj_ptr, (MapType*)storage);
    }

    static PyObject* convert(const MapType& c)
    {
        pxr::boost::python::dict result = TfPyCopyMapToDictionary(c);
        return pxr::boost::python::incref(result.ptr());
    }

private:
    static PyObject* _convert(PyObject* pyDict, MapType* result)
    {
        extract<dict> dictProxy(pyDict);
        if (!dictProxy.check()) {
            return NULL;
        }
        dict d = dictProxy();

        list keys = d.keys();
        for (int i = 0, numKeys = len(d); i < numKeys; ++i) {
            object pyKey = keys[i];
            extract<KeyType> keyProxy(pyKey);
            if (!keyProxy.check()) {
                return NULL;
            }

            object pyValue = d[pyKey];
            extract<ValueType> valueProxy(pyValue);
            if (!valueProxy.check()) {
                return NULL;
            }

            KeyType key = keyProxy();
            if (result) {
                result->insert(std::make_pair(keyProxy(), valueProxy()));
            }
        }

        return pyDict;
    }
};

class Sdf_VariantSelectionProxyWrap {
public:
    typedef SdfVariantSelectionProxy Type;
    typedef Type::key_type key_type;
    typedef Type::mapped_type mapped_type;
    typedef Type::value_type value_type;
    typedef Type::iterator iterator;
    typedef Type::const_iterator const_iterator;
    typedef std::pair<key_type, mapped_type> pair_type;

    static void SetItem(Type& x,
                        const key_type& key, const mapped_type& value)
    {
        if (value.empty()) {
            x.erase(key);
        }
        else {
            std::pair<iterator, bool> i = x.insert(value_type(key, value));
            if (!i.second && i.first != iterator()) {
                i.first->second = value;
            }
        }
    }

    static mapped_type SetDefault(Type& x, const key_type& key,
                                  const mapped_type& def)
    {
        const_iterator i = x.find(key);
        if (i != x.end()) {
            return i->second;
        }
        else if (!def.empty()) {
            SdfChangeBlock block;
            return x[key] = def;
        }
        else {
            return def;
        }
    }

    static void Update(Type& x, const std::vector<pair_type>& values)
    {
        SdfChangeBlock block;
        TF_FOR_ALL(i, values) {
            if (i->second.empty()) {
                x.erase(i->first);
            }
            else {
                x[i->first] = i->second;
            }
        }
    }

    static void UpdateDict(Type& x, const pxr::boost::python::dict& d)
    {
        UpdateList(x, d.items());
    }

    static void UpdateList(Type& x, const pxr::boost::python::list& pairs)
    {
        std::vector<pair_type> values;
        for (int i = 0, n = len(pairs); i != n; ++i) {
            values.push_back(pair_type(
                extract<key_type>(pairs[i][0]),
                extract<mapped_type>(pairs[i][1])));
        }
        Update(x, values);
    }
};

static
void
_ModifyVariantSelectionProxy()
{
    // The map edit proxy for SdfVariantSelectionProxy has to have a
    // special behavior for assignment:  assigning the empty string
    // means delete.  Rather than mess with SdfPyMapEditProxy we just
    // edit the python class, replacing the original methods with
    // customized methods.  We need to fix __setitem__, setdefault,
    // and update.
    typedef Sdf_VariantSelectionProxyWrap Wrap;
    object cls = TfPyGetClassObject<SdfVariantSelectionProxy>();

    // Erase old methods.
    PyObject* const ns = cls.ptr();
    PyObject* dict = ((PyTypeObject*)ns)->tp_dict;
    PyObject_DelItem(dict, str("__setitem__").ptr());
    PyObject_DelItem(dict, str("setdefault").ptr());
    PyObject_DelItem(dict, str("update").ptr());

    // Insert new methods.
    object setitem    = make_function(&Wrap::SetItem);
    object setdefault = make_function(&Wrap::SetDefault);
    object updateList = make_function(&Wrap::UpdateList);
    object updateDict = make_function(&Wrap::UpdateDict);
    objects::add_to_namespace(cls, "__setitem__", setitem);
    objects::add_to_namespace(cls, "setdefault", setdefault);
    objects::add_to_namespace(cls, "update", updateDict);
    objects::add_to_namespace(cls, "update", updateList);
}

static TfEnum
_DefaultUnitWrapper1(const TfEnum & unit)
{
    return SdfDefaultUnit(unit);
}

static TfEnum
_DefaultUnitWrapper2(const TfToken &typeName)
{
    return SdfDefaultUnit(typeName);
}

static string
_UnitCategoryWrapper(const TfEnum & unit)
{
    return SdfUnitCategory(unit);
}

static string
_UnregisteredValueRepr(const SdfUnregisteredValue &self)
{
    string value = TfPyRepr(self.GetValue());
    return TF_PY_REPR_PREFIX + "UnregisteredValue(" + value + ")";
}

static int
_UnregisteredValueHash(const SdfUnregisteredValue &self)
{
    const VtValue &value = self.GetValue();
    if (value.IsHolding<VtDictionary>()) {
        return VtDictionaryHash()(value.Get<VtDictionary>());
    } else if (value.IsHolding<std::string>()) {
        return TfHash()(value.Get<std::string>());
    } else {
        return 0;
    }
}

static string
_SdfValueBlockRepr(const SdfValueBlock &self) 
{
    return TF_PY_REPR_PREFIX + "ValueBlock";
}  

static int
_SdfValueBlockHash(const SdfValueBlock &self)
{
    return TfHash{}(self);
}

SdfValueTypeName
_FindType(const std::string& typeName)
{
    return SdfSchema::GetInstance().FindType(typeName);
}

pxr::boost::python::tuple
_ConvertToValidMetadataDictionary(VtDictionary dict)
{
    std::string errMsg;
    bool success = SdfConvertToValidMetadataDictionary(&dict, &errMsg);
    return pxr::boost::python::make_tuple(success, dict, errMsg);
}

} // anonymous namespace 

void wrapTypes()
{
    TF_PY_WRAP_PUBLIC_TOKENS("ValueRoleNames",
                             SdfValueRoleNames, SDF_VALUE_ROLE_NAME_TOKENS);

    def( "DefaultUnit", _DefaultUnitWrapper1,
        "For a given unit of measurement get the default compatible unit.");

    def( "DefaultUnit", _DefaultUnitWrapper2,
        "For a given typeName ('Vector', 'Point' etc.) get the "
        "default unit of measurement.");

    def( "UnitCategory", _UnitCategoryWrapper,
        "For a given unit of measurement get the unit category.");

    def( "ConvertUnit", &SdfConvertUnit,
        "Convert a unit of measurement to a compatible unit.");

    def( "ValueHasValidType", &SdfValueHasValidType );
    def( "GetTypeForValueTypeName", &SdfGetTypeForValueTypeName );
    def( "GetValueTypeNameForValue", &SdfGetValueTypeNameForValue );

    def( "ConvertToValidMetadataDictionary",
         &_ConvertToValidMetadataDictionary );

    def( "GetUnitFromName", &SdfGetUnitFromName,
         return_value_policy<return_by_value>() );
    def( "GetNameForUnit", &SdfGetNameForUnit,
         return_value_policy<return_by_value>() );

    // Register Python conversions for std::vector<SdfUnregisteredValue>
    using _UnregisteredValueVector = std::vector<SdfUnregisteredValue>;
    to_python_converter<_UnregisteredValueVector,
                        TfPySequenceToPython<_UnregisteredValueVector> >();
    TfPyContainerConversions::from_python_sequence<
        _UnregisteredValueVector,
        TfPyContainerConversions::variable_capacity_policy >();

    // Register python conversions for SdfRelocate and SdfRelocates
    to_python_converter<SdfRelocate, 
        TfPyContainerConversions::to_tuple<SdfRelocate>>();
    TfPyContainerConversions::from_python_tuple_pair<SdfRelocate>();

    to_python_converter<SdfRelocates,
                        TfPySequenceToPython<SdfRelocates> >();
    TfPyContainerConversions::from_python_sequence<
        SdfRelocates,
        TfPyContainerConversions::variable_capacity_policy >();

    TfPyWrapEnum<SdfListOpType>("ListOpType");
    TfPyWrapEnum<SdfPermission>("Permission");
    TfPyWrapEnum<SdfSpecifier>("Specifier");
    TfPyWrapEnum<SdfVariability>("Variability");
    TfPyWrapEnum<SdfSpecType>("SpecType");
    TfPyWrapEnum<SdfAuthoringError>("AuthoringError");

    VtValueFromPython<SdfListOpType>();
    VtValueFromPython<SdfPermission>();
    VtValueFromPython<SdfSpecifier>();
    VtValueFromPython<SdfVariability>();
    VtValueFromPython<SdfSpecType>();
    VtValueFromPython<SdfAuthoringError>();

    // Wrap all units enums.
    #define _WRAP_ENUM(elem)                                 \
        TfPyWrapEnum<_SDF_UNITSLIST_ENUM(elem)>();           \
        VtValueFromPython<_SDF_UNITSLIST_ENUM(elem)>();
    _SDF_FOR_EACH_UNITS(_WRAP_ENUM, _SDF_UNITS)
    #undef _WRAP_ENUM

    SdfPyWrapListProxy<SdfNameOrderProxy>();
    SdfPyWrapListProxy<SdfSubLayerProxy>();
    SdfPyWrapListEditorProxy<SdfConnectionsProxy>();
    SdfPyWrapListEditorProxy<SdfInheritsProxy>();
    SdfPyWrapListEditorProxy<SdfPayloadsProxy>();
    SdfPyWrapListEditorProxy<SdfReferencesProxy>();
    SdfPyWrapListEditorProxy<SdfVariantSetNamesProxy>();

    SdfPyWrapChildrenView<SdfAttributeSpecView>();
    SdfPyWrapChildrenView<SdfPrimSpecView>();
    SdfPyWrapChildrenView<SdfPropertySpecView>();
    SdfPyWrapChildrenView<SdfRelationalAttributeSpecView>();
    SdfPyWrapChildrenView<SdfRelationshipSpecView>();
    SdfPyWrapChildrenView<SdfVariantView>();
    SdfPyWrapChildrenView<SdfVariantSetView>();

    SdfPyWrapMapEditProxy<SdfDictionaryProxy>();
    SdfPyWrapMapEditProxy<SdfVariantSelectionProxy>();
    SdfPyWrapMapEditProxy<SdfRelocatesMapProxy>();

    SdfPyWrapListOp<SdfPathListOp>("PathListOp");
    SdfPyWrapListOp<SdfPayloadListOp>("PayloadListOp");
    SdfPyWrapListOp<SdfReferenceListOp>("ReferenceListOp");
    SdfPyWrapListOp<SdfStringListOp>("StringListOp");
    SdfPyWrapListOp<SdfTokenListOp>("TokenListOp");
    SdfPyWrapListOp<SdfIntListOp>("IntListOp");
    SdfPyWrapListOp<SdfInt64ListOp>("Int64ListOp");
    SdfPyWrapListOp<SdfUIntListOp>("UIntListOp");
    SdfPyWrapListOp<SdfUInt64ListOp>("UInt64ListOp");
    SdfPyWrapListOp<SdfUnregisteredValueListOp>("UnregisteredValueListOp");

    VtValueFromPython<SdfPathListOp>();
    VtValueFromPython<SdfPayloadListOp>();
    VtValueFromPython<SdfReferenceListOp>();
    VtValueFromPython<SdfStringListOp>();
    VtValueFromPython<SdfTokenListOp>();
    VtValueFromPython<SdfIntListOp>();
    VtValueFromPython<SdfInt64ListOp>();
    VtValueFromPython<SdfUIntListOp>();
    VtValueFromPython<SdfUInt64ListOp>();
    VtValueFromPython<SdfUnregisteredValueListOp>();

    // Modify class wrappers for special behaviors (see function comments).
    _ModifyVariantSelectionProxy();

    // Register python conversions for SdfRelocatesMap.
    Sdf_MapTypeConverter<SdfRelocatesMap>();

    // Register python conversions for SdfVariantSelectionMap.
    Sdf_MapTypeConverter<SdfVariantSelectionMap>();

    // Register python conversions for SdfTimeSampleMap.
    to_python_converter<SdfTimeSampleMap, Sdf_TimeSampleMapConverter>();

    class_<SdfUnregisteredValue>("UnregisteredValue")
        .def(init<const std::string &>())
        .def(init<const VtDictionary &>())
        .def(init<const SdfUnregisteredValue &>())
        .def(init<const SdfUnregisteredValueListOp &>())

        .add_property("value",
            make_function(&SdfUnregisteredValue::GetValue,
                return_value_policy<return_by_value>()))

        .def(self == self)
        .def(self != self)

        .def("__repr__", _UnregisteredValueRepr)
        .def("__hash__", _UnregisteredValueHash)
        ;

    VtValueFromPython<SdfUnregisteredValue>();

    class_<Sdf_ValueTypeNamesType, noncopyable>(
            "ValueTypeNames", no_init)
        .def( "Find", &_FindType )
        .staticmethod("Find")
        .def_readonly("Bool"           , SdfValueTypeNames->Bool)
        .def_readonly("UChar"          , SdfValueTypeNames->UChar)
        .def_readonly("Int"            , SdfValueTypeNames->Int)
        .def_readonly("UInt"           , SdfValueTypeNames->UInt)
        .def_readonly("Int64"          , SdfValueTypeNames->Int64)
        .def_readonly("UInt64"         , SdfValueTypeNames->UInt64)
        .def_readonly("Half"           , SdfValueTypeNames->Half)
        .def_readonly("Float"          , SdfValueTypeNames->Float)
        .def_readonly("Double"         , SdfValueTypeNames->Double)
        .def_readonly("TimeCode"       , SdfValueTypeNames->TimeCode)
        .def_readonly("String"         , SdfValueTypeNames->String)
        .def_readonly("Token"          , SdfValueTypeNames->Token)
        .def_readonly("Asset"          , SdfValueTypeNames->Asset)
        .def_readonly("Opaque"         , SdfValueTypeNames->Opaque)
        .def_readonly("Group"          , SdfValueTypeNames->Group)
        .def_readonly("PathExpression" , SdfValueTypeNames->PathExpression)
        .def_readonly("Int2"           , SdfValueTypeNames->Int2)
        .def_readonly("Int3"           , SdfValueTypeNames->Int3)
        .def_readonly("Int4"           , SdfValueTypeNames->Int4)
        .def_readonly("Half2"          , SdfValueTypeNames->Half2)
        .def_readonly("Half3"          , SdfValueTypeNames->Half3)
        .def_readonly("Half4"          , SdfValueTypeNames->Half4)
        .def_readonly("Float2"         , SdfValueTypeNames->Float2)
        .def_readonly("Float3"         , SdfValueTypeNames->Float3)
        .def_readonly("Float4"         , SdfValueTypeNames->Float4)
        .def_readonly("Double2"        , SdfValueTypeNames->Double2)
        .def_readonly("Double3"        , SdfValueTypeNames->Double3)
        .def_readonly("Double4"        , SdfValueTypeNames->Double4)
        .def_readonly("Point3h"        , SdfValueTypeNames->Point3h)
        .def_readonly("Point3f"        , SdfValueTypeNames->Point3f)
        .def_readonly("Point3d"        , SdfValueTypeNames->Point3d)
        .def_readonly("Vector3h"       , SdfValueTypeNames->Vector3h)
        .def_readonly("Vector3f"       , SdfValueTypeNames->Vector3f)
        .def_readonly("Vector3d"       , SdfValueTypeNames->Vector3d)
        .def_readonly("Normal3h"       , SdfValueTypeNames->Normal3h)
        .def_readonly("Normal3f"       , SdfValueTypeNames->Normal3f)
        .def_readonly("Normal3d"       , SdfValueTypeNames->Normal3d)
        .def_readonly("Color3h"        , SdfValueTypeNames->Color3h)
        .def_readonly("Color3f"        , SdfValueTypeNames->Color3f)
        .def_readonly("Color3d"        , SdfValueTypeNames->Color3d)
        .def_readonly("Color4h"        , SdfValueTypeNames->Color4h)
        .def_readonly("Color4f"        , SdfValueTypeNames->Color4f)
        .def_readonly("Color4d"        , SdfValueTypeNames->Color4d)
        .def_readonly("Quath"          , SdfValueTypeNames->Quath)
        .def_readonly("Quatf"          , SdfValueTypeNames->Quatf)
        .def_readonly("Quatd"          , SdfValueTypeNames->Quatd)
        .def_readonly("Matrix2d"       , SdfValueTypeNames->Matrix2d)
        .def_readonly("Matrix3d"       , SdfValueTypeNames->Matrix3d)
        .def_readonly("Matrix4d"       , SdfValueTypeNames->Matrix4d)
        .def_readonly("Frame4d"        , SdfValueTypeNames->Frame4d)
        .def_readonly("TexCoord2h"     , SdfValueTypeNames->TexCoord2h)
        .def_readonly("TexCoord2f"     , SdfValueTypeNames->TexCoord2f)
        .def_readonly("TexCoord2d"     , SdfValueTypeNames->TexCoord2d)
        .def_readonly("TexCoord3h"     , SdfValueTypeNames->TexCoord3h)
        .def_readonly("TexCoord3f"     , SdfValueTypeNames->TexCoord3f)
        .def_readonly("TexCoord3d"     , SdfValueTypeNames->TexCoord3d)

        .def_readonly("BoolArray"      , SdfValueTypeNames->BoolArray)
        .def_readonly("UCharArray"     , SdfValueTypeNames->UCharArray)
        .def_readonly("IntArray"       , SdfValueTypeNames->IntArray)
        .def_readonly("UIntArray"      , SdfValueTypeNames->UIntArray)
        .def_readonly("Int64Array"     , SdfValueTypeNames->Int64Array)
        .def_readonly("UInt64Array"    , SdfValueTypeNames->UInt64Array)
        .def_readonly("HalfArray"      , SdfValueTypeNames->HalfArray)
        .def_readonly("FloatArray"     , SdfValueTypeNames->FloatArray)
        .def_readonly("DoubleArray"    , SdfValueTypeNames->DoubleArray)
        .def_readonly("TimeCodeArray"  , SdfValueTypeNames->TimeCodeArray)
        .def_readonly("StringArray"    , SdfValueTypeNames->StringArray)
        .def_readonly("TokenArray"     , SdfValueTypeNames->TokenArray)
        .def_readonly("AssetArray"     , SdfValueTypeNames->AssetArray)
        .def_readonly("PathExpressionArray",
                      SdfValueTypeNames->PathExpressionArray)
        .def_readonly("Int2Array"      , SdfValueTypeNames->Int2Array)
        .def_readonly("Int3Array"      , SdfValueTypeNames->Int3Array)
        .def_readonly("Int4Array"      , SdfValueTypeNames->Int4Array)
        .def_readonly("Half2Array"     , SdfValueTypeNames->Half2Array)
        .def_readonly("Half3Array"     , SdfValueTypeNames->Half3Array)
        .def_readonly("Half4Array"     , SdfValueTypeNames->Half4Array)
        .def_readonly("Float2Array"    , SdfValueTypeNames->Float2Array)
        .def_readonly("Float3Array"    , SdfValueTypeNames->Float3Array)
        .def_readonly("Float4Array"    , SdfValueTypeNames->Float4Array)
        .def_readonly("Double2Array"   , SdfValueTypeNames->Double2Array)
        .def_readonly("Double3Array"   , SdfValueTypeNames->Double3Array)
        .def_readonly("Double4Array"   , SdfValueTypeNames->Double4Array)
        .def_readonly("Point3hArray"   , SdfValueTypeNames->Point3hArray)
        .def_readonly("Point3fArray"   , SdfValueTypeNames->Point3fArray)
        .def_readonly("Point3dArray"   , SdfValueTypeNames->Point3dArray)
        .def_readonly("Vector3hArray"  , SdfValueTypeNames->Vector3hArray)
        .def_readonly("Vector3fArray"  , SdfValueTypeNames->Vector3fArray)
        .def_readonly("Vector3dArray"  , SdfValueTypeNames->Vector3dArray)
        .def_readonly("Normal3hArray"  , SdfValueTypeNames->Normal3hArray)
        .def_readonly("Normal3fArray"  , SdfValueTypeNames->Normal3fArray)
        .def_readonly("Normal3dArray"  , SdfValueTypeNames->Normal3dArray)
        .def_readonly("Color3hArray"   , SdfValueTypeNames->Color3hArray)
        .def_readonly("Color3fArray"   , SdfValueTypeNames->Color3fArray)
        .def_readonly("Color3dArray"   , SdfValueTypeNames->Color3dArray)
        .def_readonly("Color4hArray"   , SdfValueTypeNames->Color4hArray)
        .def_readonly("Color4fArray"   , SdfValueTypeNames->Color4fArray)
        .def_readonly("Color4dArray"   , SdfValueTypeNames->Color4dArray)
        .def_readonly("QuathArray"     , SdfValueTypeNames->QuathArray)
        .def_readonly("QuatfArray"     , SdfValueTypeNames->QuatfArray)
        .def_readonly("QuatdArray"     , SdfValueTypeNames->QuatdArray)
        .def_readonly("Matrix2dArray"  , SdfValueTypeNames->Matrix2dArray)
        .def_readonly("Matrix3dArray"  , SdfValueTypeNames->Matrix3dArray)
        .def_readonly("Matrix4dArray"  , SdfValueTypeNames->Matrix4dArray)
        .def_readonly("Frame4dArray"   , SdfValueTypeNames->Frame4dArray)
        .def_readonly("TexCoord2hArray", SdfValueTypeNames->TexCoord2hArray)
        .def_readonly("TexCoord2fArray", SdfValueTypeNames->TexCoord2fArray)
        .def_readonly("TexCoord2dArray", SdfValueTypeNames->TexCoord2dArray)
        .def_readonly("TexCoord3hArray", SdfValueTypeNames->TexCoord3hArray)
        .def_readonly("TexCoord3fArray", SdfValueTypeNames->TexCoord3fArray)
        .def_readonly("TexCoord3dArray", SdfValueTypeNames->TexCoord3dArray)
        ;

    class_<SdfValueBlock>("ValueBlock")
        .def(self == self)
        .def(self != self)
        .def("__repr__", _SdfValueBlockRepr)
        .def("__hash__", _SdfValueBlockHash);
    VtValueFromPython<SdfValueBlock>();
}
