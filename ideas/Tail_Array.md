# Tail Arrays

Defining a tail-array class X:
* `using X = Tail_Array<X_Base>;`

The base class (argument of `Tail_Array`) must define 3 members:
* typename `value_type`, because I can't get `decltype` to work for this purpose.
* `size_t size_` data member.
* `value_type array_[0]` data member

constructors:
* `make_fill(size)` -- default initialize elements and base
* `make_fill(size, value_type, ...)`
* `make_range(input_iter, size, ...)`
* `make_elements(const collection&, ...)`
* `make_elements(collection&&, ...)`
* `make_elements(initializer_list<value_type>, ...)`

TODO: abstraction to extend the base class to implement the Container
concept for an array.

## High Level Interface
`Tail_Array<base_class,value_type>` extends an arbitrary `base_class`
with additional private data members representing a dynamic sized inline array
of `value_type` (a "tail array").

The tail array is accessed using public members:
* `size_t size() const noexcept`
* `value_type* begin() noexcept`
* `value_type* end() noexcept`
* `const value_type* begin() const noexcept`
* `const value_type* end() const noexcept`
* ... (minimum requirements for a random-access container class in C++14)

### API 0 (high level, split class definition)
This API is "high level" because the job of defining the tail array data
members is encapsulated in the `Tail_Array` template, not exposed to the user.
```
class X_Data {
    // data, constructors, destructor
};
class X : public Tail_Array<X_Data, T, X> {
    // member functions which access tail array
};
```
This seemed okay until I tried to define an `X_Data` which inherits from
a pure virtual base class. Then, trouble: the `X_Data` class must define
the final versions of virtual member functions that access the tail array,
otherwise the `X_Data` constructor will lay down the wrong vtable.

So: the X constructor must be in the same class as the tail array data
members and the member functions that access it.

### API 1
```
template <typename value_type> class Tail_Array_Data {
protected:
    size_t size_;
    value_type array_[0];
public:
    Tail_Array_Data(size_t sz) : size_(sz) {}
    size_t size() const noexcept { return size_; }
    value_type* begin() noexcept { return array_; }
    ...
};
class X_Data : public XBase, public Tail_Array_Data<T> {
    // data, constructors, destructor, member functions, everything
};
class X : public Tail_Array_Construction<X_Data, X> {};
```

## A Straw-Man High Level Interface
I want a simple, obvious, high-level interface for including a dynamic sized
tail array within a class C. Requirements:
* C can inherit from a base class B.
* C can define data members.
* There's a way to inject the tail array into C's data.
* C can define member functions that access the tail array.

It's tricky, because of the underlying
implementation constraints. The resulting class C:
* Has private constructors, but no protected or public constructors.
  This enforces some restrictions.
  * You can't construct an instance of the class in the normal way, because it
    wouldn't work (memory corruption, the array elements won't be initialized).
  * You can't derive a subclass with additional data members, and then
    instantiate the subclass (more memory corruption, the additional data
    members would overlay the array elements). Turns out I don't need
    final classes to enforce this restriction.
* Instead has static member factory functions used to heap allocate instances.
  There are no stack instances, I don't know a useable and portable way
  to implement that, although `alloca` may be involved. Static instances could
  be constructed using a `Static` member template class which is derived from C.

Inheritance must be used to get those static members into C.

This initially seems like a straight-forward interface:
```
struct My_Array : public Tail_Array<double,My_Array>
{
    double determinant;
    My_Array(size_t size, double) : Tail_Array(size), determinant(0) {}
};
```
Inheriting from `Tail_Array<T,Super>` adds a dynamic sized tail array of
type T to the end of the derived class, plus member functions for accessing it:
`size()`, `begin()`, `operator[]`, etc. You can make those member functions
public, protected, etc based on how `Tail_Array` is inherited.

The derived class C should have no public constructors.
With this interface, that's tricky, because the derived class defines
data members and needs to provide a constructor for those members to
`Tail_Array`; it's easiest if that constructor is public. So instead, maybe:
```
struct My_Array : public Tail_Array<double,My_Array>
{
    double determinant;
private:
    My_Array(size_t size, double d) : Tail_Array(size), determinant(d) {}
    friend class Tail_Array<double,My_Array>;
};
```
An alternative is
```
class My_Array_Base
{
    // define data, constructors, destructors
    double determinant;
public:
    My_Array(double d) : determinant(d) {}
};
class My_Array : public Tail_Array<double,My_Array_Base,My_Array>
{ 
    // define member functions
};
```
Specification of the Base class is simple: there are no restrictions on
the definition and it is completely independent of the implementation
requirements for tail arrays. All of the complexity is
in a line of boilerplate to construct the result class.

So with this design, you have to split up your class implementation into
two pieces. The base class (My_Array_Base) contains your data members
(minus the tail array), constructors and destructors. The derived class
(My_Array) contains member functions that access the tail array.

Maybe this form also enables you to convert those public members (make and
Static and begin/end/size/...) to protected or private via using declarations
in the class body.

And I chose that in preference to the simpler
```
class My_Array_Base
{
    double determinant;
public:
    My_Array(double d) : determinant(d) {}
};
using My_Array = Tail_Array<double,My_Array_Base>;
```
because:
* I want error messages to use the name `My_Array`.
* How does My_Array implement member functions that have access to the
  array data?

Maybe I can support both versions by providing a default for the
third template argument of `Tail_Array`.

## Class Definition
For the generalized `Tail_Array` template, my best design so far is
```
class My_Array_Base { ...; size_t size(); T array_[0]; };
class My_Array final : public Tail_Array<My_Array_Base,My_Array> {};
```
which requires an auxiliary definition of a base class.
I can't make this work:
```
class My_Array final : public Tail_Array<My_Array>
{ ...; size_t size(); T array_[0]; };
```
because the `make` factory functions in `Tail_Array` need the `My_Array`
argument to be a fully defined class, not a forward reference, since they
need the size of the class.

Hmm. Maybe I just move those function definitions out of the class body?

Also, I'd like a simplified interface that automatically adds the size and
array members, if you just want to add an array to the end of your class
with minimal configuration.
```
class My_Array final : public Tail<My_Array,value_type> { ... };
```

## Static Tail Arrays
There's no way to construct a tail array on the stack, without using alloca
(or maybe dynamic arrays), but it would be possible to construct them
statically.

A tail array class has a Static subclass that takes a size as a template
argument, and has some constructors, similar to std::array.

No way to define this inside the `Tail_Array` template and still have the
more convenient subclassing API, I think. Can I put a forward decl
`class Static;` inside the `Tail_Array` class and define it afterwards?

Foo::Static<42> st;
auto dy = Foo::make(42);

## Constructors
Tail array classes don't have public constructors.
Instead, they have public factory functions that heap allocate instances.
* `make(size, ...)` -- default construct elements. Base(size,...).
  Same as `make_fill({},size,...)`.
* `make_fill(fillval, size, ...)`
* `make_elements(elem1,elem2,...)` -- default construct base.
* or `make_elements({elem1,elem2,...},...)`
* `make_copy(iter,size,...)`
* `make_move(iter,size,...)`

std::vector constructors:
* size [,value_type]
* ifirst, ilast
* const vector&
* vector&&
* `initializer_list<value_type>`

new idea:
* `make_fill(size)` -- default initialize elements and base
* `make_fill(size, value_type, ...)`
* `make_range(input_iter, size, ...)`
* `make_elements(const collection&, ...)`
* `make_elements(collection&&, ...)`
* `make_elements(initializer_list<value_type>, ...)`

## Unique Array Pointers
I'd like to use `unique_ptr` to manage a tail array.
What's the syntax?
```
unique_ptr p {TA::make(42)};
```
You can't use `std::make_unique`, but there's no good reason to need this.

## Shared Array Pointers
Same comment for `std::shared_ptr` as for `std::unique_ptr`.
Too bad you can't use `std::make_shared`, you lose that performance advantage.

## Uniform Initialization; Initializer Lists
How would you use the uniform initialization syntax to construct a tail array
from a list of elements?

Can I even use

## `aux::Tail_Array::Builder` implementation
I've abandoned this part of the design.

What's the best algorithm for a dynamically growing array?

What's the best sequence of size values? According to Stack Overflow,
* Successive doubling gives you amortized O(1) time.
* Using a factor of phi (fibonacci sequence) gives a block size sequence
  in which previously deallocated blocks can be reused in later allocation
  sequences. A factor of 1.5 may be close enough.

On all target platforms, you can query the actual size of a malloc'ed block.
Linux: `<malloc.h>` `malloc_usable_size()`,
BSD, OSX: `<malloc/malloc.h>` `malloc_size()`, Windows: `_msize()`.
* I predict this will be some constant + some multiple of maxalign.
  So you can measure a platform and build a compile time profile of
  allocation behaviour. Instead of making these calls at runtime,
  just round up malloc sizes to the appropriate value.

So, there are 3 measures of the Tail_Array::Builder buffer size:
* The number of initialized array elements.
* The nominal `capacity`, as requested by user using `reserve(n)`,
  which is actually a lower bound since block sizes get rounded up
  to make memory management efficient.
* The actual capacity, and the actual block size (which might be bigger
  by a fraction of an element size).

And:
* There is the sequence of block sizes supported by malloc on a given platform.
* There is the behaviour when you try to shrink these block sizes.
* There is the actual subset of block sizes chosen by `Tail_Array::Builder`.
  Considerations are: amortized cost of growing an array, and reuse of
  previously freed block sizes in an future growing array, and fragmentation
  cost of wasted space at end of blocks.
* Maybe I have a fixed (global) sequence of block sizes, chosen based on
  platform parameters, maybe it's a fibonacci-like sequence, plus a fast way
  to map a minimum required capacity onto a block size from this sequence.
* This could be a separate abstraction, used by `Tail_Array::Builder`.
  `array_growth_sequence<Base,T>(capacity)` -> nbytes
* Yeah, this is a one-size-fits-all kind of design. With enough knowledge of
  application and platform behaviour, you could do better.
  But I want a simple abstraction that is "good enough", and that is as close
  to optimum as you can get for the mythical "general case", without knowledge
  of actual application behaviour (eg, via feedback from profiling).

Which C library is used by MinGW on Windows? You have a choice.

Use realloc to shrink an array and get rid of slack after it is finalized?
* On Windows 64, can't shrink a block less than 16K.
* On Darwin, it was reported in 2006 or something that blocks don't shrink,
  particularly in megabyte range.
* So test this: see if the realloc call helps, see what's appropriate behaviour
  per platform.

Using `realloc` to expand a block in place and shrink a block is something
that's not supported by C++ Allocators. Also, using Allocators, I see no
supported way to allocate an appropriately aligned tail array structure.
So I think that I can't support Allocators. This is relevant if I am going
to submit Tail_Array for consideration by Boost.

### `Tail_Array::Builder`
I was originally planning to include a `Tail_Array::Builder` class, which is
used for building an instance of `Tail_Array` one piece at a time. The API
would be similar to `std::vector`. The implementation would look a lot like
`std::vector`, except that the heap allocated array owned by the structure
would be a `Tail_Array`. That ought to be more efficient, right?

I was also planning to use the standard C idiom for building a dynamic array,
using `realloc`, because that ought to be more efficient than allocating a new
array each time it grows beyond capacity, right?

One problem with `realloc` is that using it requires that we can safely
relocate an instance of `value_type` to a new address without calling the
move constructor. That works for primitive types like integers and pointers.
It's probably unsafe for `std::string` using the small string optimization,
and some other library types. The issue is whether there's a data member
whose bit pattern depends on the bit pattern of the `this` pointer.
Eg, a data member that points to another data member.
There's no type attribute to test if this is safe.

The Builder class is a performance optimization. The alternative is to
construct a `std::vector`, then copy or move it into a `Tail_Array`.
Unless my Builder class is significantly better than this, it isn't worthwhile
to write the code. Getting this level of performance out of Builder will get
me into the realm of writing platform dependent code, where I characterize
different malloc/free implementations, and define a set of compile time
parameters for describing their behaviour and any useful non-standard extensions
like `malloc_usable_size/malloc_size/_msize`. It's a big project,
and there's no guarantee that's it worthwhile.

Here is some abandoned code:
```
public:
    friend class Builder;
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
            if (result_ != nullptr)
                result_->destroy_array(size());
        }

        value_type* begin() { return begin_; }
        value_type* end() { return end_; }
        size_t size() { return end_ - begin_; }
        size_t capacity() { return end_cap_ - begin_; }

        template<typename... Rest>
        Tail_Array* release(Rest... rest)
        {
            if (result_ == nullptr)
                return Tail_Array::make(0, rest...);
            else {
                Tail_Array* r = result_;
                size_t sz = size();
                result_ = nullptr;
                begin_ = nullptr;
                end_ = nullptr;
                end_cap_ = nullptr;
                try {
                    new((void*)r) Base(sz, rest...);
                } catch(...) {
                    r->destroy_array(sz);
                    free((void*)r);
                    throw;
                }
                return r;
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
                // Note: realloc will relocate array elements in memory
                // without calling the value_type move constructor.
                // TODO: on Linux, call malloc_usable_size() to adjust new_cap.
                // On Windows, it is size_t _msize(void*). On OSX, it's
                // malloc_size(), but instead use malloc_good_size().
                void* newmem = realloc((void*)result_,
                    sizeof(Tail_Array) + new_cap * sizeof(value_type));
                if (newmem == nullptr)
                    throw std::bad_alloc();

                size_t sz = size();
                result_ = (Tail_Array*) newmem;
                begin_ = result_->array_;
                end_ = begin_ + sz;
                end_cap_ = begin_ + new_cap;
            }
        }
    };
```
