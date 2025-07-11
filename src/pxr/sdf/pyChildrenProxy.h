// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_PY_CHILDREN_PROXY_H
#define PXR_SDF_PY_CHILDREN_PROXY_H

/// \file sdf/pyChildrenProxy.h

#include "./childrenProxy.h"
#include <pxr/arch/demangle.h>
#include <pxr/tf/pyError.h>
#include <pxr/tf/pyUtils.h>
#include <pxr/tf/stringUtils.h>
#include <pxr/boost/python.hpp>
#include <pxr/boost/python/slice.hpp>

namespace pxr {

template <class _View>
class SdfPyChildrenProxy {
public:
    typedef _View View;
    typedef SdfChildrenProxy<View> Proxy;
    typedef typename Proxy::key_type key_type;
    typedef typename Proxy::mapped_type mapped_type;
    typedef typename Proxy::mapped_vector_type mapped_vector_type;
    typedef typename Proxy::size_type size_type;
    typedef SdfPyChildrenProxy<View> This;

    SdfPyChildrenProxy(const Proxy& proxy) : _proxy(proxy)
    {
        _Init();
    }

    SdfPyChildrenProxy(const View& view, const std::string& type,
                       int permission = Proxy::CanSet |
                                        Proxy::CanInsert |
                                        Proxy::CanErase) :
        _proxy(view, type, permission)
    {
        _Init();
    }

    bool operator==(const This& other) const
    {
        return _proxy == other._proxy;
    }

    bool operator!=(const This& other) const
    {
        return _proxy != other._proxy;
    }

private:
    typedef typename Proxy::const_iterator _const_iterator;
    typedef typename View::const_iterator _view_const_iterator;

    struct _ExtractItem {
        static pxr::boost::python::object Get(const _const_iterator& i)
        {
            return pxr::boost::python::make_tuple(i->first, i->second);
        }
    };

    struct _ExtractKey {
        static pxr::boost::python::object Get(const _const_iterator& i)
        {
            return pxr::boost::python::object(i->first);
        }
    };

    struct _ExtractValue {
        static pxr::boost::python::object Get(const _const_iterator& i)
        {
            return pxr::boost::python::object(i->second);
        }
    };

    template <class E>
    class _Iterator {
    public:
        _Iterator(const pxr::boost::python::object& object) :
            _object(object),
            _owner(pxr::boost::python::extract<const This&>(object)()._proxy)
        {
            _cur = _owner.begin();
        }

        _Iterator<E> GetCopy() const
        {
            return *this;
        }

        pxr::boost::python::object GetNext()
        {
            if (_cur == _owner.end()) {
                TfPyThrowStopIteration("End of ChildrenProxy iteration");
            }
            pxr::boost::python::object result = E::Get(_cur);
            ++_cur;
            return result;
        }

    private:
        pxr::boost::python::object _object;
        const Proxy& _owner;
        _const_iterator _cur;
    };

    void _Init()
    {
        TfPyWrapOnce<This>(&This::_Wrap);
    }

    static void _Wrap()
    {
        using namespace pxr::boost::python;

        std::string name = _GetName();

        scope thisScope =
        class_<This>(name.c_str(), no_init)
            .def("__repr__", &This::_GetRepr, TfPyRaiseOnError<>())
            .def("__len__", &This::_GetSize, TfPyRaiseOnError<>())
            .def("__getitem__", &This::_GetItemByKey, TfPyRaiseOnError<>())
            .def("__getitem__", &This::_GetItemByIndex, TfPyRaiseOnError<>())
            .def("__setitem__", &This::_SetItemByKey, TfPyRaiseOnError<>())
            .def("__setitem__", &This::_SetItemBySlice, TfPyRaiseOnError<>())
            .def("__delitem__", &This::_DelItemByKey, TfPyRaiseOnError<>())
            .def("__delitem__", &This::_DelItemByIndex, TfPyRaiseOnError<>())
            .def("__contains__", &This::_HasKey, TfPyRaiseOnError<>())
            .def("__contains__", &This::_HasValue, TfPyRaiseOnError<>())
            .def("__iter__",   &This::_GetValueIterator, TfPyRaiseOnError<>())
            .def("clear", &This::_Clear, TfPyRaiseOnError<>())
            .def("append", &This::_AppendItem, TfPyRaiseOnError<>())
            .def("insert", &This::_InsertItemByIndex, TfPyRaiseOnError<>())
            .def("get", &This::_PyGet, TfPyRaiseOnError<>())
            .def("get", &This::_PyGetDefault, TfPyRaiseOnError<>())
            .def("items", &This::_GetItemIterator, TfPyRaiseOnError<>())
            .def("keys", &This::_GetKeyIterator, TfPyRaiseOnError<>())
            .def("values", &This::_GetValueIterator, TfPyRaiseOnError<>())
            .def("index", &This::_FindIndexByKey, TfPyRaiseOnError<>())
            .def("index", &This::_FindIndexByValue, TfPyRaiseOnError<>())
            .def("__eq__", &This::operator==, TfPyRaiseOnError<>())
            .def("__ne__", &This::operator!=, TfPyRaiseOnError<>())
            ;

        class_<_Iterator<_ExtractItem> >
            ((name + "_Iterator").c_str(), no_init)
            .def("__iter__", &This::template _Iterator<_ExtractItem>::GetCopy)
            .def("__next__", &This::template _Iterator<_ExtractItem>::GetNext)
            ;

        class_<_Iterator<_ExtractKey> >
            ((name + "_KeyIterator").c_str(), no_init)
            .def("__iter__", &This::template _Iterator<_ExtractKey>::GetCopy)
            .def("__next__", &This::template _Iterator<_ExtractKey>::GetNext)
            ;

        class_<_Iterator<_ExtractValue> >
            ((name + "_ValueIterator").c_str(), no_init)
            .def("__iter__", &This::template _Iterator<_ExtractValue>::GetCopy)
            .def("__next__", &This::template _Iterator<_ExtractValue>::GetNext)
            ;
    }

    static std::string _GetName()
    {
        std::string name = "ChildrenProxy_" +
                           ArchGetDemangled<View>();
        name = TfStringReplace(name, " ", "_");
        name = TfStringReplace(name, ",", "_");
        name = TfStringReplace(name, "::", "_");
        name = TfStringReplace(name, "<", "_");
        name = TfStringReplace(name, ">", "_");
        return name;
    }

    const View& _GetView() const
    {
        return _proxy._view;
    }

    View& _GetView()
    {
        return _proxy._view;
    }

    std::string _GetRepr() const
    {
        std::string result("{");
        if (! _proxy.empty()) {
            _const_iterator i = _proxy.begin(), n = _proxy.end();
            result += TfPyRepr(i->first) + ": " + TfPyRepr(i->second);
            while (++i != n) {
                result += ", " + TfPyRepr(i->first) +
                          ": " + TfPyRepr(i->second);
            }
        }
        result += "}";
        return result;
    }

    size_type _GetSize() const
    {
        return _proxy.size();
    }

    mapped_type _GetItemByKey(const key_type& key) const
    {
        _view_const_iterator i = _GetView().find(key);
        if (i == _GetView().end()) {
            TfPyThrowIndexError(TfPyRepr(key));
            return mapped_type();
        }
        else {
            return *i;
        }
    }

    mapped_type _GetItemByIndex(int index) const
    {
        index = TfPyNormalizeIndex(index, _proxy.size(), true /*throwError*/);
        return _GetView()[index];
    }

    void _SetItemByKey(const key_type& key, const mapped_type& value)
    {
        TF_CODING_ERROR("can't directly reparent a %s",
                        _proxy._GetType().c_str());
    }

    void _SetItemBySlice(const pxr::boost::python::slice& slice,
                         const mapped_vector_type& values)
    {
        if (! TfPyIsNone(slice.start()) ||
            ! TfPyIsNone(slice.stop()) ||
            ! TfPyIsNone(slice.step())) {
            TfPyThrowIndexError("can only assign to full slice [:]");
        }
        else {
            _proxy._Copy(values);
        }
    }

    void _DelItemByKey(const key_type& key)
    {
        if (_GetView().find(key) == _GetView().end()) {
            TfPyThrowIndexError(TfPyRepr(key));
        }
        _proxy._Erase(key);
    }

    void _DelItemByIndex(int index)
    {
        _proxy._Erase(_GetView().key(_GetItemByIndex(index)));
    }

    void _Clear()
    {
        _proxy._Copy(mapped_vector_type());
    }

    void _AppendItem(const mapped_type& value)
    {
        _proxy._Insert(value, _proxy.size());
    }

    void _InsertItemByIndex(int index, const mapped_type& value)
    {
        // Note that -1 below means to insert at end for the _proxy._Insert API.
        index = index < (int)_proxy.size() 
            ? TfPyNormalizeIndex(index, _proxy.size(), false /*throwError*/)
            : -1;

        _proxy._Insert(value, index);
    }

    pxr::boost::python::object _PyGet(const key_type& key) const
    {
        _view_const_iterator i = _GetView().find(key);
        return i == _GetView().end() ? pxr::boost::python::object() :
                                       pxr::boost::python::object(*i);
    }

    pxr::boost::python::object _PyGetDefault(const key_type& key,
                                        const mapped_type& def) const
    {
        _view_const_iterator i = _GetView().find(key);
        return i == _GetView().end() ? pxr::boost::python::object(def) :
                                       pxr::boost::python::object(*i);
    }

    bool _HasKey(const key_type& key) const
    {
        return _GetView().find(key) != _GetView().end();
    }

    bool _HasValue(const mapped_type& value) const
    {
        return _GetView().find(value) != _GetView().end();
    }

    static
    _Iterator<_ExtractItem> _GetItemIterator(const pxr::boost::python::object &x)
    {
        return _Iterator<_ExtractItem>(x);
    }

    static
    _Iterator<_ExtractKey> _GetKeyIterator(const pxr::boost::python::object &x)
    {
        return _Iterator<_ExtractKey>(x);
    }

    static
    _Iterator<_ExtractValue> _GetValueIterator(const pxr::boost::python::object &x)
    {
        return _Iterator<_ExtractValue>(x);
    }

    template <class E>
    pxr::boost::python::list _Get() const
    {
        pxr::boost::python::list result;
        for (_const_iterator i = _proxy.begin(), n = _proxy.end(); i != n; ++i){
            result.append(E::Get(i));
        }
        return result;
    }

    int _FindIndexByKey(const key_type& key) const
    {
        size_t i = std::distance(_GetView().begin(), _GetView().find(key));
        return i == _GetView().size() ? -1 : i;
    }

    int _FindIndexByValue(const mapped_type& value) const
    {
        size_t i = std::distance(_GetView().begin(), _GetView().find(value));
        return i == _GetView().size() ? -1 : i;
    }

private:
    Proxy _proxy;

    template <class E> friend class _Iterator;
};

}  // namespace pxr

#endif // PXR_SDF_PY_CHILDREN_PROXY_H
