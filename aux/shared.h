// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef AUX_SHARED_H
#define AUX_SHARED_H

#include <boost/intrusive_ptr.hpp>
#include <cstdint>
#include <memory>
#include <new>

namespace aux {

/// Cheap alternative to `std::shared_ptr`.
///
/// Memory overhead is one pointer, instead of two for `std::shared_ptr`.
/// For further economies, see `aux::Shared_Base`.
template<typename T>
struct Shared : public boost::intrusive_ptr<T>
{
    // restate base class constructors
    Shared() noexcept : boost::intrusive_ptr<T>() {}
    Shared(T*p) : boost::intrusive_ptr<T>(p) {}
    Shared(const Shared& r) : boost::intrusive_ptr<T>(r) {}
    template<class Y> Shared(Shared<Y> const& r) : boost::intrusive_ptr<T>(r) {}
    Shared(Shared&& rhs) noexcept : boost::intrusive_ptr<T>(rhs) {}

    // restating move constructor requires restatement of assignment ops
    Shared& operator=(Shared&& rhs) noexcept
    {
        boost::intrusive_ptr<T>::operator=(rhs);
        return *this;
    }
    Shared& operator=(Shared const& rhs)
    {
        boost::intrusive_ptr<T>::operator=(rhs);
        return *this;
    }

    // additional constructor, for parity with std::shared_ptr
    Shared(std::unique_ptr<T> uptr) : boost::intrusive_ptr<T>(uptr.release()) {}
    template<class Y> Shared(std::unique_ptr<Y> p)
    : boost::intrusive_ptr<T>(p.release())
    {}
};

/// Cheap alternative to `std::make_shared`.
template<typename T, class... Args> Shared<T> make(Args&&... args)
{
    void* raw = std::malloc(sizeof(T));
    if (raw == nullptr)
        throw std::bad_alloc();
    T* ptr;
    try {
        ptr = new(raw) T(args...);
    } catch (...) {
        std::free(raw);
        throw;
    }
    return Shared<T>(ptr);
}

/// Common base class for cheap reference-counted objects.
///
/// For performance reasons, the use_count is incremented and decremented
/// non-atomically, which is not thread safe. That's what you typically
/// need in cases where `std::shared_ptr` is too expensive.
///
/// The memory overhead is one use_count, instead of two for `std::shared_ptr`.
/// Plus I'm forcing the use of a vtable. I specifically want the vtable pointer
/// (64 bits) to precede the 32 bit use_count, for alignment reasons. This
/// leaves room for a subclass to add another 32 bits of data without creating
/// an alignment hole in the memory layout on 64 bit systems. I use this to
/// reduce memory consumption for curv::Ref_Value, which puts a 32 bit type code
/// in this location.
///
/// Suppose `Foo` is a subclass of `Shared_Base`.
/// 1. To pass an instance of `Foo` to a function `f`, pass it by reference
///    (`Foo&`), not by value (it's non-copyable), and not by `Shared<Foo>`
///    in most cases (too expensive: that would increment/decrement the
///    reference count).
/// 2. As a special case, for constructors and factory functions, where the
///    normal usage is for the caller to pass ownership of a Shared to
///    the constructor, then declare the argument as `Shared<Foo>`.
/// 3. In case #1, if `f` needs to keep a reference alive after the function
///    has returned, then it can create a Shared to the instance.
///    (That's not possible with `std::shared_ptr`.) The cost of constructing
///    a Shared is only incurred when required, and the interface to `f`
///    doesn't need to expose this implementation detail, by being changed
///    to accept a `Shared<Foo>` argument. This is particular relevant
///    when a member function needs to construct Shared<Foo>(this),
///    something impossible with std::shared_ptr.
/// 4. As a corrolary to #3, it is DANGEROUS to construct a static or auto
///    instance of `Foo`, because the technique of #3 could result in an attempt
///    to delete a static or auto variable, leading to memory corruption.
///    Therefore, all instances of `Foo` should be constructed using `make`.
/// 5. I wish I could enforce the requirement to not construct auto or static
///    instances. The class is non-copyable, but that's not enough.
///    And this is already causing problems for me.
///    * If a function takes a Foo& rather than a Shared<Foo> argument (but
///      retains a reference as in #3), then I'm tempted to construct an auto
///      variable, rather than call `make`. It's just habit. So #1 is
///      looking like bad advice (for ensuring code correctness).
///    * Protected constructors would help with this. But then, every subclass
///      needs a friend declaration for `make`.
///    * Or, we protect the Shared constructor.
///      For #1, we need a conversion from `T& param` to Shared<T>.
///      For #3, we need a conversion from `T* this` to Shared<T>.
///      So, share(T&) -> Shared<T>.
///      Instead of #4, we allow static/auto instances,
///      but we can detect them at runtime because use_count==0.
///      `share()` aborts if use_count==0.
struct Shared_Base
{
    Shared_Base() : use_count(0) {}
    virtual ~Shared_Base() {}
    mutable std::uint32_t use_count;

    // operator new and delete are defined to invoke malloc and free
    // because subclasses of Shared_Base that implement variable-length objects
    // must use malloc for their heap allocation, and we must therefore
    // consistently use free for freeing Shared_Base objects.
    void* operator new(std::size_t size)
    {
        void* p = malloc(size);
        if (p == nullptr)
            throw std::bad_alloc();
        return p;
    }
    void* operator new(std::size_t size, void* ptr) noexcept
    {
        return ptr;
    }
    void operator delete(void* p) noexcept
    {
        free(p);
    }
private:
    // Shared_Base is non-copyable.
    Shared_Base(const Shared_Base&) = delete;
    Shared_Base& operator=(const Shared_Base&) = delete;
};

inline void intrusive_ptr_add_ref(const Shared_Base* p)
{
    ++p->use_count;
}

inline void intrusive_ptr_release(const Shared_Base* p)
{
    if (--p->use_count == 0)
        delete p;
}

// How to support atomic refcount update in the future.
// * Add a Shared_Atomic_Base class, which always uses atomic updates.
// * Add a Shared_Maybe_Atomic_Base class, which reserves a bit in the
//   use_count to indicate whether the use_count should be updated atomically.
//   When an object is first constructed, it's only accessible from one thread.
//   Later, it might "escape" and become visible to more than thread.
//   If and when this happens, we mark the object as atomic.

} // namespace aux
#endif // header guard
