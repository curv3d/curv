// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef AUX_TAIL_ARRAY_H
#define AUX_TAIL_ARRAY_H

namespace aux {

/// Construct a class whose last data member is an inline variable sized array.
///
/// The `Base` class has a typename member, `value_type`,
/// plus two data members, public or protected:
/// * `size_` has an integral type.
/// * `value_type array_[0];`, declared as the last data member.
///
/// The class constructed by the template has some restrictions:
/// * You can't inherit from it (declared final).
/// * You can't assign to it, copy it or move it.
/// * You can't construct an instance using a constructor or `new`.
///   You must use the `make` member function, which allocates an instance
///   on the heap. (But you do use `delete` to free a heap instance.)
template<typename Base>
class Tail_Array final : public Base
{
public:
    /// construct a heap instance, with a specified array size
    static Tail_Array* make(decltype(Base::size_) size)
    {
        typedef typename Base::value_type T;
        void* mem = malloc(sizeof(Tail_Array) + size*sizeof(T));
        if (mem == nullptr)
            throw std::bad_alloc();
        Tail_Array* p = new(mem) Tail_Array();
        p->Base::size_ = size;
        // TODO: do nothing if Base::value_type has trivial constructor.
        for (decltype(Base::size_) i = 0; i < size; ++i)
        {
            new((void*)&p->Base::array_[i]) T();
        }
        return p;
    }
    ~Tail_Array()
    {
        typedef typename Base::value_type T;
        // TODO: do nothing if Base::value_type has trivial destructor.
        for (decltype(Base::size_) i = 0; i < Base::size_; ++i)
        {
            Base::array_[i].~T();
        }
    }
    void operator delete(void* p) noexcept
    {
        free(p);
    }

private:
    Tail_Array(Tail_Array const&) = delete;
    Tail_Array(Tail_Array &&) = delete;
    Tail_Array& operator=(Tail_Array const&) = delete;
    void* operator new(std::size_t) = delete;

    void* operator new(std::size_t size, void* ptr) noexcept
    {
        return ptr;
    }
    Tail_Array() : Base() { }

public:
    class Builder {};
};

} // namespace
#endif // header guard
