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
/// are a reduced number of allocations, improved cache locality,
/// and space savings (eliminating the array pointer and the overhead of
/// a second allocated block).
///
/// The `Base` class has the following members:
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
/// The only ways to construct instances are:
/// * `make(size_t size, ...)` where `...` are additional `Base` class
///   constructor arguments. All array elements are default constructed.
/// * Construct an instance of `Builder`, use a `std::vector`-like interface
///   to add array elements, then call `release(...)` to construct the instance,
///   where `...` are Base class constructor arguments.
///
/// Use `delete` to destroy an instance.
///
/// The class constructed by the template has some restrictions:
/// * You can't inherit from it (declared final).
/// * You can't construct an instance using a constructor or `new`.
///   There are no public constructors.
/// * You can't assign to it, copy it or move it.
template<typename Base>
class Tail_Array final : public Base
{
public:
    /// construct a heap instance, with a specified array size
    template<typename... Rest>
    static Tail_Array* make(size_t size, Rest... rest)
    {
        typedef typename Base::value_type T;
        void* mem = malloc(sizeof(Tail_Array) + size*sizeof(T));
        if (mem == nullptr)
            throw std::bad_alloc();
        Tail_Array* p = (Tail_Array*)mem;

        // first construct the array elements
        if (!std::is_trivially_default_constructible<T>::value) {
            static_assert(std::is_nothrow_default_constructible<T>::value,
                "Base::value_type default constructor must be declared noexcept");
            for (size_t i = 0; i < size; ++i)
            {
                new((void*)&p->Base::array_[i]) T();
            }
        }

        // then construct the Base
        try {
            new(mem) Base(size, rest...);
        } catch(...) {
            p->destroy_array(size);
            free(mem);
            throw;
        }
        return p;
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
        typedef typename Base::value_type T;
        if (!std::is_trivially_destructible<T>::value) {
            static_assert(std::is_nothrow_destructible<T>::value,
                "Base::value_type destructor must be declared noexcept");
            for (size_t i = 0; i < size; ++i)
            {
                Base::array_[i].~T();
            }
        }
    }

public:
#if 0
    class Builder
    {
    public:
        using value_type = typename Base::value_type;
    private:
        // data members
        // The only initialized memory in result_ are the array
        // elements in the begin_,end_ range. The rest of result_ is
        // only initialized when release() is called.
        Tail_Array* result_;
        value_type* begin_;
        value_type* end_;
        value_type* end_cap_;
    public:
        Builder()
        :
            result_(nullptr),
            begin_(nullptr),
            end_(nullptr),
            end_cap_(nullptr)
        {
        }
        ~Builder()
        {
            delete result_;
        }

        value_type* begin() { return begin_; }
        value_type* end() { return end_; }
        size_t size() { return end_ - begin_; }
        size_t capacity() { return end_cap_ - begin_; }

        Tail_Array* release() // name from unique_ptr
        {
            if (result_ == nullptr)
                return Tail_Array::make(0);
            else {
                Tail_Array* tmp = result_;
                result_ = nullptr;
                begin_ = nullptr;
                end_ = nullptr;
                end_cap_ = nullptr;
                return tmp;
            }
        }

        void push_back(value_type val)
        {
            if (end_ == end_cap_) {
                // increase capacity
                size_t new_cap = capacity() * 2;
                if (new_cap < 8)
                    new_cap = 8;
                reserve(new_cap);
            }
            *end_++ = std::move(val);
        }

        void reserve(size_t new_cap)
        {
            if (new_cap > capacity()) {
                size_t size = size();
                if (result_ == nullptr) {
                    result_ = Tail_Array::make(new_cap);
                } else {
                    void* mem = (void*)result_;
                    void* newmem = realloc(mem,
                        sizeof(Tail_Array) + new_cap * sizeof(value_type));
                }
            }
        }
    };
#endif
};

} // namespace
#endif // header guard
