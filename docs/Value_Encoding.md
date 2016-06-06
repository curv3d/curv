# Value Encoding
The C++ type curv::Value represents any Curv value: null, boolean, number,
string, list, object and function.

## NaN Boxing
Each Value is encoded in 64 bits, using NaN Boxing. Floating point values
are encoded as 64 bit doubles, non-numeric values are encoded as NaNs.
* JavaScriptCore and LuaJIT use NaN Boxing.

A 64 bit IEEE float has the following representation, from high to low order bits: 1 bit sign, 11 bit exponent, 52 bit mantissa.

A NaN has the exponent set to all 1s, and a non-zero mantissa (a zero mantissa
would encode infinity). The highest mantissa bit means signalling or non-signalling: the interpretation is processor dependent: usually 1 means quiet (intel,
arm, most others), but PA-RISC and MIPS use 0 for quiet.

The 2011 amd64 architecture supports 52 bit physical addresses and 48 bit
virtual addresses. 64 bit ARM also uses 48 bit virtual addresses.

If I use 48 bits to encode pointers (do any processors support more than 48 bits
of virtual address space?) then that leaves 16 bits at the top of the NaN,
which are: 1 (sign), 11 (exponent), 1 (quiet flag), 3 (type code).

So a 3 bit primary type code, which shouldn't be 0 (to avoid encoding infinity).
In addition, the sign bit and low order bits of a pointer could be used to
encode additional type information.

For pure arithmetic operations, I could skip type checks, and test if the
result is NaN. Or I could encode non-numeric values as signalling NaNs, and
use an exception handler to report non-numeric operands, to avoid the overhead
of a NaN check in the normal case. The exception handler only needs to abort
evaluation of the entire script.
* This is less useful when all arithmetic operations are polymorphic over lists.
  It would work for code that has been specialized for a single type
  due to JIT optimization. Or if I support optional type declarations.

null, true and false each have unique bit patterns.
The difference between true and false is a single bit, so that the other
63 bits indicate type Boolean.

string, list, object and function are the pointer types.
These complex types each have multiple representations.

There are also some non-value types that share the NaN Box representation:
Nil, RFunction, Macro.

## Reference Counting
We use reference counting, not a copying garbage collector.
The goal is a simple implementation, and a simple interface with C++ code.
(But, we use complex techniques to avoid cyclic references. So far, I think
the net complexity is still less than copying garbage collection.)

A value object has a 32 bit reference count. The evaluator is single threaded,
the refcount is not atomically updated, for speed.

## String
The fastest representation is variable sized: a length followed by characters.
That requires some infrastructure to make practical.

An easier representation to implement is <size_t,char*>.

We also want efficient slices. Due to reference counting, a slice contains
a reference to the original string.

Strings are UTF-8.

## List
The obvious implementation is an array of curv::Value.
Similar comments as for string.

I want an efficient range representation (OpenSCAD2).
```
  { size_t, Value*, Shared_Ptr<List> parent }
```

I want efficient array update using copy-on-write, destructive update of arrays
with refcount==1. For ranges, I unconditionally copy the range on write.

Efficient concat? Functional arrays which support random access, cons, head
and tail with reasonable efficiency:
* One Sided Flexible Arrays
  http://www.cs.ox.ac.uk/people/ralf.hinze/publications/ICFP02.pdf
  Multiway trees where each node consists of an array of elements
  and an array of subtrees; base array type is configurable.
* Braun Trees, all ops O(log N)
* Skew binary random-access list, log. array ops, const. list ops.
* Finger trees

I may want lazy lists. List comprehensions could be lazy. Maybe a lazy concat.
Maybe use Haskell representation of a linked list, with Value encoding for an
unevaluated thunk. Need to avoid cyclic references. I think this can be
detected and prevented at compile time.

## Object
The obvious representation is just a map from names to values,
plus a list of indexed values.
That works for some use cases, like reading a JSON file,
but doesn't work for object literals in the general case.

Names can be represented as Lisp atoms, to make lookup more efficient.
If a hash table is used, the hash can be stored in the atom.

Function bindings within an object literal can't be stored as values, due
to reference counting and recursive function definitions.
Instead we store an rfunction, which is a function minus its lexical
environment. When an rfunction is called, the lexical environment, packaged
as an object, is passed as an implicit argument.
The object value provides access to
non-local bindings outside the object literal, which are referenced by
function literals inside the object literal.

A script is conceptually evaluated into an object, but, as an optimization,
top level definitions are stored as syntax trees, which are compiled on demand
(JIT compilation).
* So, I need a C++ type which is either a Value or an Expression.
  Probably, I nan-encode expressions.

GUI metadata needs to be associated with those definitions that are GUI
controllable parameters.

Scad2 supports an obj(p1=v1,p2=v2) syntax. Each member is independently updated,
there's no support for implicit update of dependent values. So a simple map
representation will work, although the entire map is copied. (Could use
copy-on-write, but what edge case would that optimize?)

Alternate representation: a map from names to indices, and an array of values.
This can be more efficiently copied, in the case of obj(...), and it's a
more efficient environment representation (for passing to an rfunction) since
the indices can be computed at compile time.

Scad2 supports "object inheritance", via the overlay operator, and mixin values.
Overlays require us to preserve the original dependencies of the definitions.
A simple implementation:
* function literal: store an rfunction
* expression has non-local, non-builtin references: store a thunk which is
  evaluated on each reference. This also permits arbitrary recursive defs.
* otherwise, store a value (compile time constant)

An optimization: the first time a non-recursive thunk is evaluated, cache
the result. Still need to keep thunk around.

### List generators
List elements, generators, assertions and echoes. These are all evaluated
at the time that the object literal/script is evaluated.

A list element might contain a recursive reference. Eg, it might be a shape
defined by a local recursive function f passed as a parameter to a shape
operator. That might cause a circular reference and break reference counting.

To fix this, list elements might need to be represented as thunks.
This is expensive, and not necessary in the common case. So we avoid doing
this where possible. Before and after evaluating the list, we record the
object's reference count. If it has increased, then back references have
been created, and we need to do something expensive to avoid creating a
circular reference.
* Once we fix the problem, we will free the original list. At the time when we
  free this list, the object doesn't actually point to the list, so no circular
  reference actually exists, so the free will work.

What action will we take to avoid creating circular references?

1. Initially, we can just abort. During initial testing of this, we can
   identify cases where a simple representation change will avoid creating
   back references.

2. We can put all of the list generation code (minus the echoes and asserts)
   into a thunk that is evaluated each time the list elements are referenced.
   This is simple, but quite expensive.

3. Another approach is to partially evaluate the list generation code. (How?)
   The result could be a list of values, a list of thunks and values, or
   a list of thunks, values, and `each thunk` generators.

4. Or, fully evaluate the list generation code to produce a list of values.
   (Assert and echo effects also occur at this time.)
   Then, walk the value tree looking for circular references back to the base
   object. For each value in the list that contains circular references,
   rewrite it as a thunk that maps an environment to a value.
   The result is a list of values and thunks.

5. Creating a thunk from an element value seems overly expensive
   (ie, actual vmcode to recreate the value).
   The main effect of using the thunk is to deep copy the list element each
   time it is referenced. Instead of using a thunk, I could do this:
   * store the element value (I know, circular reference)
   * store NBR, the number of back references in the element value
   * subtract NBR from the object's refcount
   * when the element is referenced, deep-copy it (halting the tree walk at
     each back-reference)
   * when the object is freed, add NBR back to the object's refcount before
     releasing the element value. Or use a specialized release method that
     detects and ignores backreferences: this eliminates the need to store NBR,
     but now we need a bit in the NaN box as a flag.

## Function
A design for functions with positional and labeled parameters but no
special variables. It considers LLVM and tail calls, but not GLSL.

### static vs dynamic functions
A function is static if all of its external references are
static (known at compile time). This includes builtin functions,
and some user-defined functions.
For dynamic functions, we need closures.

An interesting question is whether any user defined function can be static.
It depends on the API for customizing an object. OpenSCAD2 admits that there
are two cases: cheap customization (plugging new values into object slots)
vs expensive customization (requires run-time compilation).
If a definition within an object is declared to be `@param`, then it is cheaply
customizable, but references to that name are non-static.
Without @param, if a definition is compile time constant, then it is static.

### static functions
A static function has an unboxed representation as the address of a
typed function. We know the number of arguments, and sometimes also the unboxed
argument types and result type. This is reflected in the function prototype.
Eg,
* `double sqrt(double)` for builtin.sqrt
* `Value f(Value,Value)` for a user defined function with two arguments,
  and no explicit or inferred type annotations. We can do this because of LLVM.
  This gives us an efficient calling convention that allows arguments to
  be passed in registers, and is tail-call compatible (eg, the function can
  overwrite its arguments in a self tail call). By contrast, a varargs
  calling convention like `Value apply(unsigned argc, Value* argv)`
  is not tail-call compatible, and also less efficient.

A builtin function is a static function for which we have the unboxed entry
point and boxed value available at analysis time.
It is represented by a Constant containing:
* result and parameter types. Eg, `double sqrt(double)`.
* unboxed entry point, eg `&sqrt`.
  Or, code for generating an LLVM operation node for this function
  (eg, the function could be an LLVM primitive).
* boxed Value

When calling a boxed function value, we may have both positional and labeled
arguments, which need to be decoded into pure positional form.
This processing is only needed if labeled arguments are present.
If enough type information is available, we can do this at compile time.
So we do the processing at compile time or at the call site before
calling the boxed entry point.

Similarly, if only positional arguments are present, then we do an argc check
based on function metadata (at compile or run time) before calling the boxed
entry point.

The boxed entry point:
* With LLVM, it would be cool if the boxed entry point (for a 2 argument
  function) could be `Value apply(Value,Value)`, allowing registers to be used.
  But: I don't know the number of arguments until run time, so this would
  require generating machine code at call time.
* So, the boxed entry point is `Value apply(Value* argv)`.
  The argv is stack allocated with the correct size based on function metadata.
  This works for any kind of evaluator (tree, byte code, LLVM).
* The function knows the number of arguments at compile time.
  This is a tail-call friendly calling convention,
  since the function can overwrite its arguments.

Therefore, a boxed function value contains:
* List of parameter specs. len(list) is # of parameters.
  This list is constructed at compile time.
  A parameter spec contains:
  * parameter name, an atom with pointer equality
  * bool: has default value
  * other parameter metadata that could be added in the future
* List of default values, constructed either at compile time or at run time
  when function value is constructed. `null` is the don't care value.
  (Note, this representation doesn't support default values that depend on
  other parameter values.)
* Boxed entry point: `Value apply(Value* argv)`.

The boxed entry point converts boxed arguments to unboxed arguments,
throwing type errors, as needed, before calling unboxed entry point.

If `apply` is a virtual function, then there are two cache hits to acquire
the function pointer. It's better to store the function pointer as
a data member, that's only one cache hit.

How does the unboxed calling convention deal with reference counting?
The called function releases argument references.
* Hand coded static functions will pass Value by value in C++. That's simple.
* The caller must bump refcounts on call, unless it can transfer ownership:
  This is appropriate if argument is last use of a named temporary,
  or argument is an rvalue, or on a tail call.
* Compatible with an optimization: mutate a value if refcount==1.
  Eg, O(1) array element update.

How does the boxed calling convention deal with reference counting?
For now, caller owns argv and caller releases.

### dynamic functions (closures)

A closure is represented by a (environment,rfunction) pair.
* The environment is an object used to retrieve the values of
  non-static, non-local bindings.
* An rfunction is a static function with an extra argument,
  the environment pointer.

I expect TeaCAD will have 3 function-like value types.
* static function
* closure
* parameterized object

TODO: We'll need a generic code sequence for calling any value.

## Macro
A builtin can be bound to a macro, which isn't a value, which is invoked
like a function but the call is resolved at compile time.
