// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_PY_MAP_EDIT_PROXY_H
#define PXR_SDF_PY_MAP_EDIT_PROXY_H

/// \file sdf/pyMapEditProxy.h

#include "./changeBlock.h"
#include <pxr/arch/demangle.h>
#include <pxr/tf/iterator.h>
#include <pxr/tf/pyUtils.h>
#include <pxr/tf/stringUtils.h>
#include <pxr/boost/python.hpp>

namespace pxr {

template <class T>
class SdfPyWrapMapEditProxy {
public:
    typedef T Type;
    typedef typename Type::key_type key_type;
    typedef typename Type::mapped_type mapped_type;
    typedef typename Type::value_type value_type;
    typedef typename Type::iterator iterator;
    typedef typename Type::const_iterator const_iterator;
    typedef SdfPyWrapMapEditProxy<Type> This;

    SdfPyWrapMapEditProxy()
    {
        TfPyWrapOnce<Type>(&This::_Wrap);
    }

private:
    typedef std::pair<key_type, mapped_type> pair_type;

    struct _ExtractItem {
        static pxr::boost::python::object Get(const const_iterator& i)
        {
            return pxr::boost::python::make_tuple(i->first, i->second);
        }
    };

    struct _ExtractKey {
        static pxr::boost::python::object Get(const const_iterator& i)
        {
            return pxr::boost::python::object(i->first);
        }
    };

    struct _ExtractValue {
        static pxr::boost::python::object Get(const const_iterator& i)
        {
            return pxr::boost::python::object(i->second);
        }
    };

    template <class E>
    class _Iterator {
    public:
        _Iterator(const pxr::boost::python::object& object) :
            _object(object),
            _owner(pxr::boost::python::extract<const Type&>(object)),
            _cur(_owner.begin()),
            _end(_owner.end())
        {
            // Do nothing
        }

        _Iterator<E> GetCopy() const
        {
            return *this;
        }

        pxr::boost::python::object GetNext()
        {
            if (_cur == _end) {
                TfPyThrowStopIteration("End of MapEditProxy iteration");
            }
            pxr::boost::python::object result = E::Get(_cur);
            ++_cur;
            return result;
        }

    private:
        pxr::boost::python::object _object;
        const Type& _owner;
        const_iterator _cur;
        const_iterator _end;
    };

    static void _Wrap()
    {
        using namespace pxr::boost::python;

        std::string name = _GetName();

        scope thisScope =
        class_<Type>(name.c_str())
            .def("__repr__", &This::_GetRepr)
            .def("__str__", &This::_GetStr)
            .def("__len__", &Type::size)
            .def("__getitem__", &This::_GetItem)
            .def("__setitem__", &This::_SetItem)
            .def("__delitem__", &This::_DelItem)
            .def("__contains__", &This::_HasKey)
            .def("__iter__",   &This::_GetKeyIterator)
            .def("values", &This::_GetValueIterator)
            .def("keys",   &This::_GetKeyIterator)
            .def("items",  &This::_GetItemIterator)
            .def("clear", &Type::clear)
            .def("get", &This::_PyGet)
            .def("get", &This::_PyGetDefault)
            .def("pop", &This::_Pop)
            .def("popitem", &This::_PopItem)
            .def("setdefault", &This::_SetDefault)
            .def("update", &This::_UpdateDict)
            .def("update", &This::_UpdateList)
            .def("copy", &This::_Copy)
            .add_property("expired", &Type::IsExpired)
            .def("__bool__", &This::_IsValid)
            .def(self == self)
            .def(self != self)
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
        std::string name = "MapEditProxy_" +
                           ArchGetDemangled<typename Type::Type>();
        name = TfStringReplace(name, " ", "_");
        name = TfStringReplace(name, ",", "_");
        name = TfStringReplace(name, "::", "_");
        name = TfStringReplace(name, "<", "_");
        name = TfStringReplace(name, ">", "_");
        return name;
    }

    static std::string _GetRepr(const Type& x)
    {
        std::string arg;
        if (x) {
            arg = TfStringPrintf("<%s>", x._Location().c_str());
        }
        else {
            arg = "<invalid>";
        }
        return TF_PY_REPR_PREFIX + _GetName() + "(" + arg + ")";
    }

    static std::string _GetStr(const Type& x)
    {
        std::string result("{");
        if (x && ! x.empty()) {
            const_iterator i = x.begin(), n = x.end();
            result += TfPyRepr(i->first) + ": " + TfPyRepr(i->second);
            while (++i != n) {
                result +=", " + TfPyRepr(i->first) + ": " + TfPyRepr(i->second);
            }
        }
        result += "}";
        return result;
    }

    static mapped_type _GetItem(const Type& x, const key_type& key)
    {
        const_iterator i = x.find(key);
        if (i == x.end()) {
            TfPyThrowKeyError(TfPyRepr(key));
            return mapped_type();
        }
        else {
            return i->second;
        }
    }

    static void _SetItem(Type& x, const key_type& key, const mapped_type& value)
    {
        std::pair<typename Type::iterator, bool> i =
            x.insert(value_type(key, value));
        if (! i.second && i.first != typename Type::iterator()) {
            i.first->second = value;
        }
    }

    static void _DelItem(Type& x, const key_type& key)
    {
        x.erase(key);
    }

    static bool _HasKey(const Type& x, const key_type& key)
    {
        return x.count(key) != 0;
    }

    static _Iterator<_ExtractItem> 
    _GetItemIterator(const pxr::boost::python::object& x)
    {
        return _Iterator<_ExtractItem>(x);
    }

    static _Iterator<_ExtractKey> 
    _GetKeyIterator(const pxr::boost::python::object& x)
    {
        return _Iterator<_ExtractKey>(x);
    }

    static _Iterator<_ExtractValue> 
    _GetValueIterator(const pxr::boost::python::object& x)
    {
        return _Iterator<_ExtractValue>(x);
    }

    static pxr::boost::python::object _PyGet(const Type& x, const key_type& key)
    {
        const_iterator i = x.find(key);
        return i == x.end() ? pxr::boost::python::object() :
                              pxr::boost::python::object(i->second);
    }

    static mapped_type _PyGetDefault(const Type& x, const key_type& key,
                                     const mapped_type& def)
    {
        const_iterator i = x.find(key);
        return i == x.end() ? def : i->second;
    }

    template <class E>
    static pxr::boost::python::list _Get(const Type& x)
    {
        pxr::boost::python::list result;
        for (const_iterator i = x.begin(), n = x.end(); i != n; ++i) {
            result.append(E::Get(i));
        }
        return result;
    }

    static mapped_type _Pop(Type& x, const key_type& key)
    {
        iterator i = x.find(key);
        if (i == x.end()) {
            TfPyThrowKeyError(TfPyRepr(key));
            return mapped_type();
        }
        else {
            mapped_type result = i->second;
            x.erase(i);
            return result;
        }
    }

    static pxr::boost::python::tuple _PopItem(Type& x)
    {
        if (x.empty()) {
            TfPyThrowKeyError("MapEditProxy is empty");
            return pxr::boost::python::tuple();
        }
        else {
            iterator i = x.begin();
            value_type result = *i;
            x.erase(i);
            return pxr::boost::python::make_tuple(result.first, result.second);
        }
    }

    static mapped_type _SetDefault(Type& x, const key_type& key,
                                   const mapped_type& def)
    {
        const_iterator i = x.find(key);
        if (i != x.end()) {
            return i->second;
        }
        else {
            return x[key] = def;
        }
    }

    static void _Update(Type& x, const std::vector<pair_type>& values)
    {
        SdfChangeBlock block;
        TF_FOR_ALL(i, values) {
            x[i->first] = i->second;
        }
    }

    static void _UpdateDict(Type& x, const pxr::boost::python::dict& d)
    {
        _UpdateList(x, d.items());
    }

    static void _UpdateList(Type& x, const pxr::boost::python::list& pairs)
    {
        using namespace pxr::boost::python;

        std::vector<pair_type> values;
        for (int i = 0, n = len(pairs); i != n; ++i) {
            values.push_back(pair_type(
                extract<key_type>(pairs[i][0])(),
                extract<mapped_type>(pairs[i][1])()));
        }
        _Update(x, values);
    }

    static void _Copy(Type& x, const typename Type::Type& other)
    {
        x = other;
    }

    static bool _IsValid(const Type& x)
    {
        return static_cast<bool>(x);
    }
};

}  // namespace pxr

#endif // PXR_SDF_PY_MAP_EDIT_PROXY_H
