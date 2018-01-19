// Copyright Doug Moen 2016-2018.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_TAIL_ARRAY_H
#define CURV_TAIL_ARRAY_H

#include <type_traits>
#include <cstdlib>
#include <cstring>
#include <new>
#include <utility>
#include <memory>

namespace curv {

// A call to this macro must be the very last statement in the tail array
// class definition, because array_ must be the last data member.
#define TAIL_ARRAY_MEMBERS(T) \
protected: \
    size_t size_; \
    T array_[0]; \
public: \
    using value_type = T; \
 \
    size_t size() const noexcept { return size_; } \
    bool empty() const noexcept { return size_ == 0; } \
 \
    value_type* begin() noexcept { return array_; } \
    value_type* end() noexcept { return array_ + size_; } \
    const value_type* begin() const noexcept { return array_; } \
    const value_type* end() const noexcept { return array_ + size_; } \
 \
    value_type& operator[](size_t i) { return array_[i]; } \
    const value_type& operator[](size_t i) const { return array_[i]; } \
    value_type& at(size_t i) { return array_[i]; } \
    const value_type& at(size_t i) const { return array_[i]; } \
 \
    value_type& front() { return array_[0]; } \
    const value_type& front() const { return array_[0]; } \
    value_type& back() { return array_[size_-1]; } \
    const value_type& back() const { return array_[size_-1]; } \

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
/// of the cookie, and you can't access the size information. Since you
/// typically need to remember the size of the array, you have to store a second
/// copy of the size elsewhere. With Tail_Array, you precisely specify the
/// cookie using the Base class. For example, your tail array class can be a
/// subclass of a common polymorphic base class, and the cookie can contain a
/// vtable pointer. Or, the cookie can contain an intrusive reference count.
///
/// To define a class A containing a tail array of element type T:
/// ```
/// class Base : ... {
///     ...
///     using value_type = T;
///     size_t size_;
///     value_type array_[0];
/// };
/// using A = Tail_Array<Base>;
/// ```
///
/// The `Base` class must define a type name `value_type` and two data members,
/// `size_` and `array_` (as protected or public). The `Base` class constructors
/// are not responsible for initializing these two data members.
/// * `value_type` is the type of the array elements.
/// * `size_` holds the number of elements in the array.
/// * `array_` is declared as the last data member.
///   This array is magically extended at construction time to contain the
///   required number of elements, which is why it must be the last data
///   member. This declaration relies on a non-standard extension to C++
///   (zero length arrays) which is enabled by default in gcc, clang and msvc++.
///
/// All instances of the class are allocated on the heap.
/// The only way to construct instances are the `make` factory functions,
/// which return std::unique_ptr. (The pointers are deleted using `delete`,
/// but that happens internal to unique_ptr.)
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
/// The `Tail_Array` template restricts the `Base` class to support safe
/// construction of instances.
/// * The class is declared final. You can't inherit from it, because adding
///   additional data members after `array_` would cause undefined behaviour.
/// * All of the constructors are private. You can only construct instances
///   using the supplied factory functions, because ordinary C++ constructors
///   don't support variable-size objects.
/// * You can't assign to it, copy it or move it.
template<class Base>
class Tail_Array final : public Base
{
    using _value_type = typename Base::value_type;
public:
    /// Allocate an instance. Array elements are default constructed.
    template<typename... Rest>
    static std::unique_ptr<Tail_Array> make(size_t size, Rest&&... rest)
    {
        // allocate the object
        void* mem = malloc(sizeof(Tail_Array) + size*sizeof(_value_type));
        if (mem == nullptr)
            throw std::bad_alloc();
        Tail_Array* r = (Tail_Array*)mem;

        // construct the array elements
        if (!std::is_trivially_default_constructible<_value_type>::value) {
            static_assert(
                std::is_nothrow_default_constructible<_value_type>::value,
                "value_type default constructor must be declared noexcept");
            for (size_t i = 0; i < size; ++i)
            {
                new((void*)&r->Base::array_[i]) _value_type();
            }
        }

        // then construct the rest of the object
        try {
            new(mem) Tail_Array(std::forward<Rest>(rest)...);
            r->Base::size_ = size;
        } catch(...) {
            r->destroy_array(size);
            free(mem);
            throw;
        }
        return std::unique_ptr<Tail_Array>(r);
    }

    /// Allocate an instance. Move elements from another collection.
    template<class C, typename... Rest>
    static std::unique_ptr<Tail_Array> make_elements(C&& c, Rest&&... rest)
    {
        // allocate the object
        auto size = c.size();
        void* mem = malloc(sizeof(Tail_Array) + size*sizeof(_value_type));
        if (mem == nullptr)
            throw std::bad_alloc();
        Tail_Array* r = (Tail_Array*)mem;

        // construct the array elements
        decltype(size) i = 0;
        auto ptr = c.begin();
        try {
            while (i < size) {
                new((void*)&r->Base::array_[i]) _value_type();
                std::swap(r->Base::array_[i], *ptr);
                ++ptr;
                ++i;
            }
        } catch (...) {
            r->destroy_array(i);
            free(mem);
            throw;
        }

        // then construct the rest of the object
        try {
            new(mem) Tail_Array(std::forward<Rest>(rest)...);
            r->Base::size_ = size;
        } catch(...) {
            r->destroy_array(size);
            free(mem);
            throw;
        }
        return std::unique_ptr<Tail_Array>(r);
    }

    /// Allocate an instance. Copy array elements from another array.
    template<typename... Rest>
    static std::unique_ptr<Tail_Array> make_copy(_value_type* a, size_t size, Rest&&... rest)
    {
        // allocate the object
        void* mem = malloc(sizeof(Tail_Array) + size*sizeof(_value_type));
        if (mem == nullptr)
            throw std::bad_alloc();
        Tail_Array* r = (Tail_Array*)mem;

        // construct the array elements
        if (std::is_trivially_copy_constructible<_value_type>::value) {
            memcpy((void*)r->Base::array_, (void*)a, size*sizeof(_value_type));
        } else if (std::is_nothrow_copy_constructible<_value_type>::value) {
            for (size_t i = 0; i < size; ++i)
            {
                new((void*)&r->Base::array_[i]) _value_type(a[i]);
            }
        } else {
            size_t i = 0;
            try {
                while (i < size) {
                    new((void*)&r->Base::array_[i]) _value_type(a[i]);
                    ++i;
                }
            } catch (...) {
                r->destroy_array(i);
                free(mem);
                throw;
            }
        }

        // then construct the rest of the object
        try {
            new(mem) Tail_Array(std::forward<Rest>(rest)...);
            r->Base::size_ = size;
        } catch(...) {
            r->destroy_array(size);
            free(mem);
            throw;
        }
        return std::unique_ptr<Tail_Array>(r);
    }

    /// Allocate an instance from an initializer list.
    template<typename... Rest>
    static std::unique_ptr<Tail_Array>
    make(std::initializer_list<_value_type> il, Rest&&... rest)
    {
        // TODO: much code duplication here.
        // allocate the object
        void* mem = malloc(sizeof(Tail_Array) + il.size()*sizeof(_value_type));
        if (mem == nullptr)
            throw std::bad_alloc();
        Tail_Array* r = (Tail_Array*)mem;

        // construct the array elements
        if (std::is_nothrow_copy_constructible<_value_type>::value) {
            size_t i = 0;
            for (auto& e : il) {
                new((void*)&r->Base::array_[i]) _value_type(e);
                ++i;
            }
        } else {
            size_t i = 0;
            try {
                for (auto& e : il) {
                    new((void*)&r->Base::array_[i]) _value_type(e);
                    ++i;
                }
            } catch (...) {
                r->destroy_array(i);
                free(mem);
                throw;
            }
        }

        // then construct the rest of the object
        try {
            new(mem) Tail_Array(std::forward<Rest>(rest)...);
            r->Base::size_ = il.size();
        } catch(...) {
            r->destroy_array(il.size());
            free(mem);
            throw;
        }
        return std::unique_ptr<Tail_Array>(r);
    }

    ~Tail_Array()
    {
        destroy_array(Base::size_);
    }
    void operator delete(void* p) noexcept
    {
        free(p);
    }

private:
    using Base::Base; // no public constructors
    Tail_Array& operator=(Tail_Array const&) = delete;
    Tail_Array& operator=(Tail_Array &&) = delete;
    void* operator new(std::size_t) = delete;

    void* operator new(std::size_t size, void* ptr) noexcept
    {
        return ptr;
    }

    void destroy_array(size_t size)
    {
        if (!std::is_trivially_destructible<_value_type>::value) {
            static_assert(std::is_nothrow_destructible<_value_type>::value,
                "value_type destructor must be declared noexcept");
            for (size_t i = 0; i < size; ++i)
            {
                Base::array_[i].~_value_type();
            }
        }
    }
};

} // namespace
#endif // header guard
