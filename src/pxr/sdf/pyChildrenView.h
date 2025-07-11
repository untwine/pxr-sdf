// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_PY_CHILDREN_VIEW_H
#define PXR_SDF_PY_CHILDREN_VIEW_H

/// \file sdf/pyChildrenView.h

#include "./childrenView.h"
#include <pxr/arch/demangle.h>
#include <pxr/tf/pyUtils.h>
#include <pxr/tf/stringUtils.h>
#include <pxr/boost/python.hpp>

namespace pxr {

template <class _View>
class SdfPyWrapChildrenView {
public:
    typedef _View View;
    typedef typename View::ChildPolicy ChildPolicy;
    typedef typename View::Predicate Predicate;
    typedef typename View::key_type key_type;
    typedef typename View::value_type value_type;
    typedef typename View::const_iterator const_iterator;
    typedef SdfPyWrapChildrenView<View> This;

    SdfPyWrapChildrenView()
    {
        TfPyWrapOnce<View>(&This::_Wrap);
    }

private:
    struct _ExtractItem {
        static pxr::boost::python::object Get(const View& x, const const_iterator& i)
        {
            return pxr::boost::python::make_tuple(x.key(i), *i);
        }
    };

    struct _ExtractKey {
        static pxr::boost::python::object Get(const View& x, const const_iterator& i)
        {
            return pxr::boost::python::object(x.key(i));
        }
    };

    struct _ExtractValue {
        static pxr::boost::python::object Get(const View& x, const const_iterator& i)
        {
            return pxr::boost::python::object(*i);
        }
    };

    template <class E>
    class _Iterator {
    public:
        _Iterator(const pxr::boost::python::object& object) :
            _object(object),
            _owner(pxr::boost::python::extract<const View&>(object)),
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
                TfPyThrowStopIteration("End of ChildrenProxy iteration");
            }
            pxr::boost::python::object result = E::Get(_owner, _cur);
            ++_cur;
            return result;
        }

    private:
        pxr::boost::python::object _object;
        const View& _owner;
        const_iterator _cur;
        const_iterator _end;
    };

    static void _Wrap()
    {
        using namespace pxr::boost::python;

        std::string name = _GetName();
        
        // Note: Using the value iterator for the __iter__ method is not
        //       consistent with Python dicts (which iterate over keys).
        //       However, we're emulating TfPyKeyedVector, which iterates
        //       over values as a vector would.
        scope thisScope =
        class_<View>(name.c_str(), no_init)
            .def("__repr__", &This::_GetRepr)
            .def("__len__", &View::size)
            .def("__getitem__", &This::_GetItemByKey)
            .def("__getitem__", &This::_GetItemByIndex)
            .def("get", &This::_PyGet)
            .def("__contains__", &This::_HasKey)
            .def("__contains__", &This::_HasValue)
            .def("__iter__",   &This::_GetValueIterator)
            .def("items", &This::_GetItemIterator)
            .def("keys", &This::_GetKeyIterator)
            .def("values", &This::_GetValueIterator)
            .def("index", &This::_FindIndexByKey)
            .def("index", &This::_FindIndexByValue)
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
        std::string name = "ChildrenView_" +
                           ArchGetDemangled<ChildPolicy>() + "_" +
                           ArchGetDemangled<Predicate>();
        name = TfStringReplace(name, " ", "_");
        name = TfStringReplace(name, ",", "_");
        name = TfStringReplace(name, "::", "_");
        name = TfStringReplace(name, "<", "_");
        name = TfStringReplace(name, ">", "_");
        return name;
    }

    static std::string _GetRepr(const View& x)
    {
        std::string result("{");
        if (! x.empty()) {
            const_iterator i = x.begin(), n = x.end();
            result += TfPyRepr(x.key(i)) + ": " + TfPyRepr(*i);
            while (++i != n) {
                result += ", " + TfPyRepr(x.key(i)) + ": " + TfPyRepr(*i);
            }
        }
        result += "}";
        return result;
    }

    static value_type _GetItemByKey(const View& x, const key_type& key)
    {
        const_iterator i = x.find(key);
        if (i == x.end()) {
            TfPyThrowIndexError(TfPyRepr(key));
            return value_type();
        }
        else {
            return *i;
        }
    }

    static value_type _GetItemByIndex(const View& x, size_t index)
    {
        if (index >= x.size()) {
            TfPyThrowIndexError("list index out of range");
        }
        return x[index];
    }

    static pxr::boost::python::object _PyGet(const View& x, const key_type& key)
    {
        const_iterator i = x.find(key);
        return i == x.end() ? pxr::boost::python::object() :
                              pxr::boost::python::object(*i);
    }

    static bool _HasKey(const View& x, const key_type& key)
    {
        return x.find(key) != x.end();
    }

    static bool _HasValue(const View& x, const value_type& value)
    {
        return x.find(value) != x.end();
    }

    static
    _Iterator<_ExtractItem> _GetItemIterator(const pxr::boost::python::object& x)
    {
        return _Iterator<_ExtractItem>(x);
    }

    static
    _Iterator<_ExtractKey> _GetKeyIterator(const pxr::boost::python::object& x)
    {
        return _Iterator<_ExtractKey>(x);
    }

    static
    _Iterator<_ExtractValue> _GetValueIterator(const pxr::boost::python::object& x)
    {
        return _Iterator<_ExtractValue>(x);
    }

    template <class E>
    static pxr::boost::python::list _Get(const View& x)
    {
        pxr::boost::python::list result;
        for (const_iterator i = x.begin(), n = x.end(); i != n; ++i) {
            result.append(E::Get(x, i));
        }
        return result;
    }

    static int _FindIndexByKey(const View& x, const key_type& key)
    {
        size_t i = std::distance(x.begin(), x.find(key));
        return i == x.size() ? -1 : i;
    }

    static int _FindIndexByValue(const View& x, const value_type& value)
    {
        size_t i = std::distance(x.begin(), x.find(value));
        return i == x.size() ? -1 : i;
    }
};

}  // namespace pxr

#endif // PXR_SDF_PY_CHILDREN_VIEW_H
