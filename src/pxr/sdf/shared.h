// Copyright 2016 Pixar
//
// Licensed under the terms set forth in the LICENSE.txt file available at
// https://openusd.org/license.
//
// Modified by Jeremy Retailleau.

#ifndef PXR_SDF_SHARED_H
#define PXR_SDF_SHARED_H

#include "pxr/sdf/pxr.h"
#include "pxr/sdf/api.h"
#include <pxr/tf/delegatedCountPtr.h>
#include <pxr/tf/hash.h>

#include <atomic>

SDF_NAMESPACE_OPEN_SCOPE


// Implementation storage + refcount for Sdf_Shared.
template <class T>
struct Sdf_Counted {
    constexpr Sdf_Counted() : count(0) {}
    explicit Sdf_Counted(T const &data) : data(data), count(0) {}
    explicit Sdf_Counted(T &&data) : data(std::move(data)), count(0) {}
    
    friend inline void
    TfDelegatedCountIncrement(Sdf_Counted const *c) {
        c->count.fetch_add(1, std::memory_order_relaxed);
    }
    friend inline void
    TfDelegatedCountDecrement(Sdf_Counted const *c) noexcept {
        if (c->count.fetch_sub(1, std::memory_order_release) == 1) {
            std::atomic_thread_fence(std::memory_order_acquire);
            delete c;
        }
    }

    T data;
    mutable std::atomic_int count;
};

struct Sdf_EmptySharedTagType {};
constexpr Sdf_EmptySharedTagType Sdf_EmptySharedTag{};

// This class provides a simple way to share a data object between clients.  It
// can be used to do simple copy-on-write, etc.
template <class T>
struct Sdf_Shared
{
    // Construct a Sdf_Shared with a value-initialized T instance.
    Sdf_Shared() : _held(TfMakeDelegatedCountPtr<Sdf_Counted<T>>()) {}
    // Create a copy of \p obj.
    explicit Sdf_Shared(T const &obj) :
        _held(TfMakeDelegatedCountPtr<Sdf_Counted<T>>(obj)) {}
    // Move from \p obj.
    explicit Sdf_Shared(T &&obj) :
        _held(TfMakeDelegatedCountPtr<Sdf_Counted<T>>(std::move(obj))) {}

    // Create an empty shared, which may not be accessed via Get(),
    // GetMutable(), IsUnique(), Clone(), or MakeUnique().  This is useful when
    // using the insert() or emplace() methods on associative containers, to
    // avoid allocating a temporary in case the object is already present in the
    // container.
    Sdf_Shared(Sdf_EmptySharedTagType) {}

    // Return a const reference to the shared data.
    T const &Get() const { return _held->data; }
    // Return a mutable reference to the shared data.
    T &GetMutable() const { return _held->data; }
    // Return true if no other Sdf_Shared instance shares this instance's data.
    bool IsUnique() const { return _held->count == 1; }
    // Make a new copy of the held data and refer to it.
    void Clone() { _held = TfMakeDelegatedCountPtr<Sdf_Counted<T>>(Get()); }
    // Ensure this Sdf_Shared instance has unique data.  Equivalent to:
    // \code
    // if (not shared.IsUnique()) { shared.Clone(); }
    // \endcode
    void MakeUnique() { if (!IsUnique()) Clone(); }

    // Equality and inequality.
    bool operator==(Sdf_Shared const &other) const {
        return _held == other._held || _held->data == other._held->data;
    }
    bool operator!=(Sdf_Shared const &other) const { return *this != other; }

    // Swap.
    void swap(Sdf_Shared &other) { _held.swap(other._held); }
    friend inline void swap(Sdf_Shared &l, Sdf_Shared &r) { l.swap(r); }

    // hash_value.
    friend inline size_t hash_value(Sdf_Shared const &sh) {
        return TfHash()(sh._held->data);
    }
private:
    TfDelegatedCountPtr<Sdf_Counted<T>> _held;
};


SDF_NAMESPACE_CLOSE_SCOPE

#endif // PXR_SDF_SHARED_H
