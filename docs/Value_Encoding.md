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
Here's a simple native function representation:
* array of parameter names
* C++ function pointer: `Value (*f)(Value* args)`.
  The number of arguments passed is always equal to the number of parameter
  names.

A compiled function is represented as an RFunction (which is not a value,
but which I expect will be nan-encoded).
An RFunction contains an array of parameter descriptors, and a vmcode block.
Each parameter descriptor has a name, an optional default value,
and in the future, additional attributes and metadata.
* Note, this representation doesn't support default values that depend on
  other parameter values.

An RFunction is invoked with:
* an array of argument values, which are probably referenced via a
  global data stack pointer
* an environment: an object used to find the values of non-local bindings

A closure is a (environment,rfunction) pair--this is a function value.

## Macro
A builtin can be bound to a macro, which isn't a value, which is invoked
like a function but the call is resolved at compile time.
