// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SHAREDV_H
#define LIBCURV_SHAREDV_H

#include <libcurv/context.h>
#include <libcurv/value.h>
#include <boost/intrusive_ptr.hpp>
//#include <cstdint>
//#include <cstdlib>
#include <memory>
#include <new>

namespace curv {

// SharedV<T> where T is a subclass of Ref_Value.
//
// This is a Curv value, with Curv value semantics,
// where the value is represented by a refcounted T* pointer.
// It implements most of the Concrete_Value interface (except ==).
// Equality is missing because reactive values don't have == equality.
// TODO: conditionally include operator== if T::operator== exists.
//
// Despite its similarity to smart pointers, SharedV has value semantics,
// not pointer semantics.
//   == is value equality, not pointer equality
//   << prints the value, not the pointer
template<typename T>
struct SharedV : protected boost::intrusive_ptr<T>
{
private:
    template<typename U, class... Args>
      friend SharedV<U> makev(Args&&... args);
    template<class U>
      friend SharedV<U> sharedv(U& obj);
public:
    // These must be public due to a GCC bug.
    SharedV(T*p) : boost::intrusive_ptr<T>(p) {}
    T* get() const { return boost::intrusive_ptr<T>::get(); }
public:
    // C++ value interface
    SharedV(const SharedV& r) : SharedV(r.get()) {}
    template<class Y> SharedV(SharedV<Y> const& r) : SharedV(r.get()) {}
    SharedV(SharedV&& rhs) noexcept : boost::intrusive_ptr<T>(rhs) {}
    SharedV& operator=(SharedV const& rhs)
    {
        boost::intrusive_ptr<T>::operator=(rhs);
        return *this;
    }
    SharedV& operator=(SharedV&& rhs) noexcept
    {
        boost::intrusive_ptr<T>::operator=(rhs);
        return *this;
    }
    void swap( SharedV& r ) noexcept
    {
        boost::intrusive_ptr<T>::swap(r);
    }

    // Concrete_Value interface (except for ==)
    SharedV() noexcept : boost::intrusive_ptr<T>() {}
    SharedV(Value v, const At_Syntax& cx) : SharedV(std::move(v.to<T>(cx))) {}
    static SharedV from_value(Value v, const At_Syntax&)
      { return {v.maybe<T>()}; }
    static SharedV from_value(Value v, Fail fl, const At_Syntax&cx)
      { return {v.to<T>(fl,cx)}; }
    Value to_value() const { return Value{Shared<T>(this->get())}; }
    void print_repr(std::ostream& o, Prec p) const { (**this).print_repr(o,p); }
    void print_string(std::ostream& o) const { (**this).print_string(o); }
    friend std::ostream& operator<<(std::ostream& o, SharedV rhs)
      { rhs.print_string(o); return o; }
    explicit operator bool() const noexcept { return this->get() != nullptr; }
    bool operator!() const noexcept { return this->get() == nullptr; }
    bool has_value() const noexcept { return get() != nullptr; }

    // SharedV interface
    SharedV(std::unique_ptr<T> uptr)
      : boost::intrusive_ptr<T>(uptr.release()) {}
    template<class Y> SharedV(std::unique_ptr<Y> p)
      : boost::intrusive_ptr<T>(p.release()) {}
    SharedV(Shared<T> p) : boost::intrusive_ptr<T>(std::move(p)) {}
    template<class Y> SharedV(Shared<Y> p)
      : boost::intrusive_ptr<T>(std::move(p)) {}
    operator Shared<T>() const noexcept { return Shared<T>(this->get()); }
    static SharedV from_value(Value v)
      { return {v.maybe<T>()}; }
    template<class U> bool isa() { return (**this).subtype_ == U::subtype; }
    template<class U> SharedV<U> cast() {
        if ((**this).subtype_ == U::subtype)
            return SharedV<U>((U*)this->get());
        else
            return SharedV<U>();
    }
    T* operator->() const noexcept
      { return boost::intrusive_ptr<T>::operator->(); }
    T& operator*() const noexcept
      { return boost::intrusive_ptr<T>::operator*(); }
};

template<typename T, class... Args>
inline SharedV<T> makev(Args&&... args)
{
    void* raw = std::malloc(sizeof(T));
    if (raw == nullptr)
        throw std::bad_alloc();
    T* ptr;
    try {
        ptr = new(raw) T(std::forward<Args>(args)...);
    } catch (...) {
        std::free(raw);
        throw;
    }
    return SharedV<T>(ptr);
}

template<class T>
inline SharedV<T> sharedv(T& obj)
{
    assert(obj.use_count > 0);
    return SharedV<T>(&obj);
}

} // namespace curv
#endif // header guard
