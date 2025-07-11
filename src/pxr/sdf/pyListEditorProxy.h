// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_PY_LIST_EDITOR_PROXY_H
#define PXR_SDF_PY_LIST_EDITOR_PROXY_H

/// \file sdf/pyListEditorProxy.h

#include "./listEditorProxy.h"
#include "./listOp.h"
#include "./pyListProxy.h"

#include <pxr/arch/demangle.h>
#include <pxr/tf/diagnostic.h>
#include <pxr/tf/pyCall.h>
#include <pxr/tf/pyResultConversions.h>
#include <pxr/tf/pyLock.h>
#include <pxr/tf/pyUtils.h>
#include <pxr/tf/stringUtils.h>
#include <pxr/boost/python.hpp>

namespace pxr {

class Sdf_PyListEditorUtils {
public:
    template <class T, class V>
    class ApplyHelper {
    public:
        ApplyHelper(const T& owner, const pxr::boost::python::object& callback) :
            _owner(owner),
            _callback(callback)
        {
            // Do nothing
        }

        std::optional<V> operator()(SdfListOpType op, const V& value)
        {
            using namespace pxr::boost::python;

            TfPyLock pyLock;
            object result = _callback(_owner, value, op);
            if (! TfPyIsNone(result)) {
                extract<V> e(result);
                if (e.check()) {
                    return std::optional<V>(e());
                }
                else {
                    TF_CODING_ERROR("ApplyEditsToList callback has "
                                    "incorrect return type.");
                }
            }
            return std::optional<V>();
        }

    private:
        const T& _owner;
        TfPyCall<pxr::boost::python::object> _callback;
    };

    template <class V>
    class ModifyHelper {
    public:
        ModifyHelper(const pxr::boost::python::object& callback) :
            _callback(callback)
        {
            // Do nothing
        }

        std::optional<V> operator()(const V& value)
        {
            using namespace pxr::boost::python;

            TfPyLock pyLock;
            object result = _callback(value);
            if (! TfPyIsNone(result)) {
                extract<V> e(result);
                if (e.check()) {
                    return std::optional<V>(e());
                }
                else {
                    TF_CODING_ERROR("ModifyItemEdits callback has "
                                    "incorrect return type.");
                }
            }
            return std::optional<V>();
        }

    private:
        TfPyCall<pxr::boost::python::object> _callback;
    };
};

template <class T>
class SdfPyWrapListEditorProxy {
public:
    typedef T Type;
    typedef typename Type::TypePolicy TypePolicy;
    typedef typename Type::value_type value_type;
    typedef typename Type::value_vector_type value_vector_type;
    typedef typename Type::ApplyCallback ApplyCallback;
    typedef typename Type::ModifyCallback ModifyCallback;
    typedef SdfPyWrapListEditorProxy<Type> This;
    typedef SdfListProxy<TypePolicy> ListProxy;

    SdfPyWrapListEditorProxy()
    {
        TfPyWrapOnce<Type>(&This::_Wrap);
        SdfPyWrapListProxy<ListProxy>();
    }

private:
    static void _Wrap()
    {
        using namespace pxr::boost::python;

        class_<Type>(_GetName().c_str(), no_init)
            .def("__str__", &This::_GetStr)
            .add_property("isExpired", &Type::IsExpired)
            .add_property("explicitItems",
                &Type::GetExplicitItems,
                &This::_SetExplicitProxy)
            .add_property("addedItems",
                &Type::GetAddedItems,
                &This::_SetAddedProxy)
            .add_property("prependedItems",
                &Type::GetPrependedItems,
                &This::_SetPrependedProxy)
            .add_property("appendedItems",
                &Type::GetAppendedItems,
                &This::_SetAppendedProxy)
            .add_property("deletedItems",
                &Type::GetDeletedItems,
                &This::_SetDeletedProxy)
            .add_property("orderedItems",
                &Type::GetOrderedItems,
                &This::_SetOrderedProxy)
            .def("GetAddedOrExplicitItems", &Type::GetAppliedItems,
                return_value_policy<TfPySequenceToTuple>()) // deprecated
            .def("GetAppliedItems", &Type::GetAppliedItems,
                return_value_policy<TfPySequenceToTuple>())
            .add_property("isExplicit", &Type::IsExplicit)
            .add_property("isOrderedOnly", &Type::IsOrderedOnly)
            .def("ApplyEditsToList",
                &This::_ApplyEditsToList,
                return_value_policy<TfPySequenceToList>())
            .def("ApplyEditsToList",
                &This::_ApplyEditsToList2,
                return_value_policy<TfPySequenceToList>())

            .def("CopyItems", &Type::CopyItems)
            .def("ClearEdits", &Type::ClearEdits)
            .def("ClearEditsAndMakeExplicit", &Type::ClearEditsAndMakeExplicit)
            .def("ContainsItemEdit", &Type::ContainsItemEdit,
                 (arg("item"), arg("onlyAddOrExplicit")=false))
            .def("RemoveItemEdits", &Type::RemoveItemEdits)
            .def("ReplaceItemEdits", &Type::ReplaceItemEdits)
            .def("ModifyItemEdits", &This::_ModifyEdits)

            // New API (see bug 8710)
            .def("Add", &Type::Add)
            .def("Prepend", &Type::Prepend)
            .def("Append", &Type::Append)
            .def("Remove", &Type::Remove)
            .def("Erase", &Type::Erase)
            ;
    }

    static std::string _GetName()
    {
        std::string name = "ListEditorProxy_" +
                           ArchGetDemangled<TypePolicy>();
        name = TfStringReplace(name, " ", "_");
        name = TfStringReplace(name, ",", "_");
        name = TfStringReplace(name, "::", "_");
        name = TfStringReplace(name, "<", "_");
        name = TfStringReplace(name, ">", "_");
        return name;
    }

    static std::string _GetStr(const Type& x)
    {
        return x._listEditor ? TfStringify(*x._listEditor) : std::string();
    }

    static void _SetExplicitProxy(Type& x, const value_vector_type& v)
    {
        x.GetExplicitItems() = v;
    }

    static void _SetAddedProxy(Type& x, const value_vector_type& v)
    {
        x.GetAddedItems() = v;
    }

    static void _SetPrependedProxy(Type& x, const value_vector_type& v)
    {
        x.GetPrependedItems() = v;
    }

    static void _SetAppendedProxy(Type& x, const value_vector_type& v)
    {
        x.GetAppendedItems() = v;
    }

    static void _SetDeletedProxy(Type& x, const value_vector_type& v)
    {
        x.GetDeletedItems() = v;
    }

    static void _SetOrderedProxy(Type& x, const value_vector_type& v)
    {
        x.GetOrderedItems() = v;
    }

    static value_vector_type _ApplyEditsToList(const Type& x,
                                               const value_vector_type& v)
    {
        value_vector_type tmp = v;
        x.ApplyEditsToList(&tmp);
        return tmp;
    }

    static value_vector_type _ApplyEditsToList2(const Type& x,
                                                const value_vector_type& v,
                                                const pxr::boost::python::object& cb)
    {
        value_vector_type tmp = v;
        x.ApplyEditsToList(&tmp,
            Sdf_PyListEditorUtils::ApplyHelper<Type, value_type>(x, cb));
        return tmp;
    }

    static void _ModifyEdits(Type& x, const pxr::boost::python::object& cb)
    {
        x.ModifyItemEdits(Sdf_PyListEditorUtils::ModifyHelper<value_type>(cb));
    }
};

}  // namespace pxr

#endif // PXR_SDF_PY_LIST_EDITOR_PROXY_H
