// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef AUX_TAIL_ARRAY_H
#define AUX_TAIL_ARRAY_H

#include <type_traits>

namespace aux {

/// Construct a class whose last data member is an inline variable sized array.
///
/// The performance benefits of putting an array inline with other data members
/// are a reduced number of allocations, improved cache locality, and space
/// savings, when compared to storing a pointer to a separately allocated
/// array.
///
/// Tail_Array is an alternative to using operator `new[]`. The latter heap
/// allocates an array prefixed with a 'cookie' that contains the size, and
/// perhaps other information. You have no control over the size or contents
/// of the cookie, and you can't access the size information. If you need to
/// remember the size of the array, you have to store a second copy of the size
/// elsewhere. With Tail_Array, you precisely specify the cookie using the Base
/// class. For example, your tail array class can be a subclass of a common
/// polymorphic base class, and the cookie can contain a vtable pointer.
/// For another example, the cookie can contain an intrusive reference count.
///
/// To define a new tail array class:
/// ```
/// class Base { ... };
/// class My_Array final : public aux::Tail_Array<Base,My_Array> {};
/// ```
///
/// The `Base` class has the following requirements:
/// * `value_type`, the type of the array elements.
///   It has a `noexcept` default constructor and a `noexcept` destructor.
/// * `value_type array_[0];`, declared as the last data member.
///   This array is magically extended at construction time to contain the
///   required number of elements, which is why it must be the last data
///   member. This declaration relies on a non-standard extension to C++
///   which is enabled by default in gcc, clang and msvc++.
/// * One or more constructors whose first argument is `size_t size`,
///   specifying the number of elements in the array. The array elements
///   are initialized in `array_` before the `Base` constructor is called.
///   A `Base` constructor may throw an exception.
/// * `size_t size()`, the number of elements in the array.
///
/// All instances of the class are allocated on the heap.
/// The only way to construct instances are the `make` factory functions.
///
/// Use `delete` to destroy an instance.
///
/// `Tail_Array` uses `malloc` and `free` for storage management.
/// This is because C++ allocators don't provide an appropriate interface:
/// there's no way to allocate a Tail_Array object while requesting
/// the correct number of bytes and the correct alignment.
///
/// Suppose `Base` is derived from a polymorphic base class `P`, such that
/// you can delete a `P*`. A big clue is that `P` defines a virtual destructor.
/// Then `P` must override operator `new` and `delete` to use `malloc` and `free`.
///
/// The class constructed by the template has some restrictions:
/// * You can't inherit from it (it should be declared final).
/// * You can't construct an instance using a constructor or `new`.
///   There are no public constructors.
/// * You can't assign to it, copy it or move it.
template<typename Base, typename Super>
class Tail_Array : public Base
{
public:
    using value_type = typename Base::value_type;

    /// Allocate an instance. Array elements are default constructed.
    template<typename... Rest>
    static Super* make(size_t size, Rest... rest)
    {
        // allocate the object
        void* mem = malloc(sizeof(Tail_Array) + size*sizeof(value_type));
        if (mem == nullptr)
            throw std::bad_alloc();
        Tail_Array* r = (Tail_Array*)mem;

        // construct the array elements
        if (!std::is_trivially_default_constructible<value_type>::value) {
            static_assert(
                std::is_nothrow_default_constructible<value_type>::value,
                "value_type default constructor must be declared noexcept");
            for (size_t i = 0; i < size; ++i)
            {
                new((void*)&r->Base::array_[i]) value_type();
            }
        }

        // then construct the Base
        try {
            new(mem) Base(size, rest...);
        } catch(...) {
            r->destroy_array(size);
            free(mem);
            throw;
        }
        return (Super*)r;
    }

    /// Allocate an instance. Copy array elements from another array.
    template<typename... Rest>
    static Super* make_copy(value_type* a, size_t size, Rest... rest)
    {
        // allocate the object
        void* mem = malloc(sizeof(Tail_Array) + size*sizeof(value_type));
        if (mem == nullptr)
            throw std::bad_alloc();
        Tail_Array* r = (Tail_Array*)mem;

        // construct the array elements
        if (std::is_trivially_copy_constructible<value_type>::value) {
            memcpy((void*)r->Base::array_, (void*)a, size*sizeof(value_type));
        } else if (std::is_nothrow_copy_constructible<value_type>::value) {
            for (size_t i = 0; i < size; ++i)
            {
                new((void*)&r->Base::array_[i]) value_type(a[i]);
            }
        } else {
            size_t i = 0;
            try {
                while (i < size) {
                    new((void*)&r->Base::array_[i]) value_type(a[i]);
                    ++i;
                }
            } catch (...) {
                r->destroy_array(i);
                free(mem);
                throw;
            }
        }

        // then construct the Base
        try {
            new(mem) Base(size, rest...);
        } catch(...) {
            r->destroy_array(size);
            free(mem);
            throw;
        }
        return (Super*)r;
    }

    ~Tail_Array()
    {
        destroy_array(Base::size());
    }
    void operator delete(void* p) noexcept
    {
        free(p);
    }

private:
    Tail_Array(Tail_Array const&) = delete;
    Tail_Array(Tail_Array &&) = delete;
    Tail_Array& operator=(Tail_Array const&) = delete;
    Tail_Array() : Base() { } // no public constructors
    void* operator new(std::size_t) = delete;

    void* operator new(std::size_t size, void* ptr) noexcept
    {
        return ptr;
    }

    void destroy_array(size_t size)
    {
        if (!std::is_trivially_destructible<value_type>::value) {
            static_assert(std::is_nothrow_destructible<value_type>::value,
                "value_type destructor must be declared noexcept");
            for (size_t i = 0; i < size; ++i)
            {
                Base::array_[i].~value_type();
            }
        }
    }
};

} // namespace
#endif // header guard
