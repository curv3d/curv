# reference counting smart pointers

I'm using boost::intrusive_ptr instead of std::shared_ptr,
for efficiency reasons:
* smaller footprint
* fewer allocations
* faster refcount manipulation (I'm using non-atomic ref counts)
* zero overhead for the equivalent of `enable_shared_from_this`,
* eliminates the possibility of the bug where two refcounts are allocated
  for the same object.

Even with this "efficient" implementation, reference counting in C++ is
kind of expensive, but the overhead of `std::shared_ptr` is hideous.

I'm aliasing `intrusive_ptr` to `Shared` since `Shared_Ptr` is too verbose.

C++ has a design flaw where `Shared<T>(new T(x))` can cause a storage leak
if used as a general expression. The C++ committee fix is `make_shared`,
but that isn't compatible with `Tail_Array` classes, which don't use `new`.
In Curv, there are three situations where a shared pointer is constructed:
* `Shared<Record> record {new<Record>()};`
* `Shared<List> list {List::make(n)};
* `Shared<T> self{this};`

I'd like to use a uniform notation for constructing shared pointers,
one which is orthogonal to the means of constructing the raw pointer.
This notation also needs to be safe. For now, initializing a local variable
will be that idiom.
* Maybe Tail_Array::make should return a unique_ptr, which is move converted
  to the smart pointer of your choice. Use make_unique or equivalent to wrap
  `new`, and ditto.

C++ best practice is to use unique_ptr where possible, since shared_ptr
is often overkill. A shared_ptr can be move constructed from a unique_ptr.
Does this make sense in Curv?
* Yes (see above) if my implementation of aux::Shared can have a constructor
  that takes a unique_ptr.
* `template<T,Rest> auto make<T>(Rest...) -> unique_ptr<T>`
  This is my standard wrapper for `new`.

Other open source projects have built their own reference counting smart
pointers, with unique features and a collection of design patterns.
Research those and steal anything that would benefit Curv. Eg,
* https://webkit.org/blog/5381/refptr-basics/
