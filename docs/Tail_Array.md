# `aux::Tail_Array::Builder` implementation

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

## `Tail_Array::Builder`
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
and other library types. There's no type attribute to test if this is safe.

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
