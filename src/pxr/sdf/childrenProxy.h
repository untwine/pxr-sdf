// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_CHILDREN_PROXY_H
#define PXR_SDF_CHILDREN_PROXY_H

/// \file sdf/childrenProxy.h

#include "./api.h"
#include "./changeBlock.h"
#include <pxr/vt/value.h>
#include <pxr/tf/diagnostic.h>
#include <pxr/tf/iterator.h>

#include <iterator>
#include <map>
#include <utility>

namespace pxr {

template <class _View>
class SdfChildrenProxy {
public:
    typedef _View View;
    typedef typename View::Adapter Adapter;
    typedef typename View::ChildPolicy ChildPolicy;
    typedef typename View::key_type key_type;
    typedef typename View::value_type mapped_type;
    typedef std::vector<mapped_type> mapped_vector_type;
    typedef std::pair<const key_type, mapped_type> value_type;
    typedef std::map<key_type, mapped_type> map_type;
    typedef typename View::size_type size_type;
    typedef SdfChildrenProxy<View> This;

private:
    typedef typename View::const_iterator _inner_iterator;

    class _ValueProxy {
    public:
        _ValueProxy() : _owner(NULL) { }
        _ValueProxy(This* owner, _inner_iterator i) : _owner(owner), _pos(i)
        {
            // Do nothing
        }

        operator mapped_type() const
        {
            return *_pos;
        }

        bool operator==(const mapped_type& other) const
        {
            return *_pos == other;
        }

    private:
        This* _owner;
        _inner_iterator _pos;
    };

    class _PairProxy {
    public:
        explicit _PairProxy(This* owner, _inner_iterator i) :
                        first(owner->_view.key(i)), second(owner, i) { }

        const key_type first;
        _ValueProxy second;

        operator value_type() const
        {
            return value_type(first, second);
        }
    };
    friend class _PairProxy;

    class _Traits {
    public:
        static value_type Dereference(const This* owner, _inner_iterator i)
        {
            return value_type(owner->_view.key(i), *i);
        }

        static _PairProxy Dereference(This* owner, _inner_iterator i)
        {
            return _PairProxy(owner, i);
        }
    };

    template <class _Owner, class _Iter, class _Value>
    class _Iterator {
        class _PtrProxy {
        public:
            _Value* operator->() { return &_value; }
        private:
            friend class _Iterator;
            explicit _PtrProxy(const _Value& value) : _value(value) {}
            _Value _value;
        };
    public:
        static_assert(!std::is_reference<_Value>::value &&
                      !std::is_pointer<_Value>::value,
                      "_Value cannot be a pointer or reference type.");
        using iterator_category = std::bidirectional_iterator_tag;
        using value_type = _Value;
        using reference = _Value;
        using pointer = _PtrProxy;
        using difference_type = std::ptrdiff_t;

        _Iterator() = default;
        _Iterator(_Owner owner, _inner_iterator i) : _owner(owner), _pos(i) { }
        template <class O2, class I2, class V2>
        _Iterator(const _Iterator<O2, I2, V2>& other) :
            _owner(other._owner), _pos(other._pos) { }

        reference operator*() const { return dereference(); }
        pointer operator->() const { return pointer(dereference()); }

        _Iterator& operator++() {
            increment();
            return *this;
        }

        _Iterator& operator--() {
            decrement();
            return *this;
        }

        _Iterator operator++(int) {
            _Iterator result(*this);
            increment();
            return result;
        }

        _Iterator operator--(int) {
            _Iterator result(*this);
            decrement();
            return result;
        }

        template <class O2, class I2, class V2>
        bool operator==(const _Iterator<O2, I2, V2>& other) const {
            return equal(other);
        }

        template <class O2, class I2, class V2>
        bool operator!=(const _Iterator<O2, I2, V2>& other) const {
            return !equal(other);
        }

    private:
        _Value dereference() const
        {
            return _Traits::Dereference(_owner, _pos);
        }

        template <class O2, class I2, class V2>
        bool equal(const _Iterator<O2, I2, V2>& other) const
        {
            return _pos == other._pos;
        }

        void increment() {
            ++_pos;
        }

        void decrement() {
            --_pos;
        }

    private:
        _Owner _owner;
        _inner_iterator _pos;

        template <class O2, class I2, class V2>
        friend class _Iterator;
    };

public:
    typedef _ValueProxy reference;
    typedef _Iterator<This*, _inner_iterator, _PairProxy> iterator;
    typedef Tf_ProxyReferenceReverseIterator<iterator> reverse_iterator;
    typedef _Iterator<const This*, _inner_iterator, value_type> const_iterator;
    typedef Tf_ProxyReferenceReverseIterator<const_iterator> const_reverse_iterator;

    static const int CanSet    = 1;
    static const int CanInsert = 2;
    static const int CanErase  = 4;

    SdfChildrenProxy(const View& view, const std::string& type,
                     int permission = CanSet | CanInsert | CanErase) :
        _view(view), _type(type), _permission(permission)
    {
        // Do nothing
    }

    template <class U>
    SdfChildrenProxy(const SdfChildrenProxy<U>& other) :
        _view(other._view), _type(other._type), _permission(other._permission)
    {
        // Do nothing
    }

    This& operator=(const This& other)
    {
        if (other._Validate()) {
            _Copy(other._view.values());
        }
        return *this;
    }

    template <class U>
    This& operator=(const SdfChildrenProxy<U>& other)
    {
        if (other._Validate()) {
            _Copy(other._view.values());
        }
        return *this;
    }

    This& operator=(const mapped_vector_type& values)
    {
        _Copy(values);
        return *this;
    }

    operator mapped_vector_type() const
    {
        return _Validate() ? _view.values() : mapped_vector_type();
    }

    map_type items() const
    {
        return _Validate() ? _view.template items_as<map_type>() :map_type();
    }

    iterator begin()
    {
        return iterator(_GetThis(), _view.begin());
    }
    iterator end()
    {
        return iterator(_GetThis(), _view.end());
    }
    const_iterator begin() const
    {
        return const_iterator(_GetThis(), _view.begin());
    }
    const_iterator end() const
    {
        return const_iterator(_GetThis(), _view.end());
    }

    reverse_iterator rbegin()
    {
        return reverse_iterator(end());
    }
    reverse_iterator rend()
    {
        return reverse_iterator(begin());
    }
    const_reverse_iterator rbegin() const
    {
        return reverse_iterator(end());
    }
    const_reverse_iterator rend() const
    {
        return reverse_iterator(begin());
    }

    size_type size() const
    {
        return _Validate() ? _view.size() : 0;
    }

    size_type max_size() const
    {
        return _view.max_size();
    }

    bool empty() const
    {
        return _Validate() ? _view.empty() : true;
    }

    std::pair<iterator, bool> insert(const mapped_type& value)
    {
        if (_Validate(CanInsert)) {
            iterator i = find(_view.key(value));
            if (i == end()) {
                if (_PrimInsert(value, size())) {
                    return std::make_pair(find(_view.key(value)), true);
                }
                else {
                    return std::make_pair(end(), false);
                }
            }
            else {
                return std::make_pair(i, false);
            }
        }
        else {
            return std::make_pair(iterator(), false);
        }
    }

    iterator insert(iterator pos, const mapped_type& value)
    {
        return insert(value).first;
    }

    template <class InputIterator>
    void insert(InputIterator first, InputIterator last)
    {
        if (_Validate(CanInsert)) {
            SdfChangeBlock block;
            for (; first != last; ++first) {
                _PrimInsert(*first, size());
            }
        }
    }

    void erase(iterator pos)
    {
        _Erase(pos->first);
    }

    size_type erase(const key_type& key)
    {
        return _Erase(key) ? 1 : 0;
    }

    void erase(iterator first, iterator last)
    {
        if (_Validate(CanErase)) {
            SdfChangeBlock block;
            while (first != last) {
                const key_type& key = first->first;
                ++first;
                _PrimErase(key);
            }
        }
    }

    void clear()
    {
        _Copy(mapped_vector_type());
    }

    iterator find(const key_type& key)
    {
        return _Validate() ? iterator(this, _view.find(key)) : iterator();
    }

    const_iterator find(const key_type& key) const
    {
        return _Validate() ? const_iterator(this, _view.find(key)) :
                             const_iterator();
    }

    size_type count(const key_type& key) const
    {
        return _Validate() ? _view.count(key) : 0;
    }

    bool operator==(const This& other) const
    {
        return _view == other._view;
    }

    bool operator!=(const This& other) const
    {
        return !(*this == other);
    }

    /// Explicit bool conversion operator. The proxy object converts to 
    /// \c true if it is valid, \c false otherwise.
    explicit operator bool() const
    {
        return _view.IsValid();
    }

private:
    const std::string& _GetType() const
    {
        return _type;
    }

    int _GetPermission() const
    {
        return _permission;
    }

    This* _GetThis()
    {
        return _Validate() ? this : NULL;
    }

    const This* _GetThis() const
    {
        return _Validate() ? this : NULL;
    }

    bool _Validate() const
    {
        if (_view.IsValid()) {
            return true;
        }
        else {
            TF_CODING_ERROR("Accessing expired %s", _type.c_str());
            return false;
        }
    }

    bool _Validate(int permission)
    {
        if (!_Validate()) {
            return false;
        }
        if ((_permission & permission) == permission) {
            return true;
        }
        const char* op = "edit";
        if (~_permission & permission & CanSet) {
            op = "replace";
        }
        else if (~_permission & permission & CanInsert) {
            op = "insert";
        }
        else if (~_permission & permission & CanErase) {
            op = "remove";
        }
        TF_CODING_ERROR("Cannot %s %s", op, _type.c_str());
        return false;
    }

    bool _Copy(const mapped_vector_type& values)
    {
        return _Validate(CanSet) ? _PrimCopy(values) : false;
    }

    bool _Insert(const mapped_type& value, size_t index)
    {
        return _Validate(CanInsert) ? _PrimInsert(value, index) : false;
    }

    bool _Erase(const key_type& key)
    {
        return _Validate(CanErase) ? _PrimErase(key) : false;
    }

    bool _PrimCopy(const mapped_vector_type& values)
    {
        typedef std::vector<typename ChildPolicy::ValueType> 
            ChildrenValueVector;

        ChildrenValueVector v;
        for (size_t i = 0; i < values.size(); ++i)
            v.push_back(Adapter::Convert(values[i]));

        return _view.GetChildren().Copy(v, _type);
    }

    bool _PrimInsert(const mapped_type& value, size_t index)
    {
        return _view.GetChildren().Insert(
            Adapter::Convert(value), index, _type);
    }

    bool _PrimErase(const key_type& key)
    {
        return _view.GetChildren().Erase(key, _type);
    }

private:
    View _view;
    std::string _type;
    int _permission;

    template <class V> friend class SdfChildrenProxy;
    template <class V> friend class SdfPyChildrenProxy;
};

// Allow TfIteration over children proxies.
template <typename _View>
struct Tf_ShouldIterateOverCopy<SdfChildrenProxy<_View> > : std::true_type
{
};

// Cannot get from a VtValue except as the correct type.
template <class _View>
struct Vt_DefaultValueFactory<SdfChildrenProxy<_View> > {
    static Vt_DefaultValueHolder Invoke() = delete;
};

}  // namespace pxr

#endif // PXR_SDF_CHILDREN_PROXY_H
