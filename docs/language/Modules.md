# Modules
A Module is a set of mutually recursive definitions, which may optionally
be parameterized, and which may optionally contain an ordered sequence
of anonymous values.

Modules exist to support
[modular programming](https://en.wikipedia.org/wiki/Modular_programming).
A Curv script file (`*.curv`) is a module.
Module files may reference one another using URLs.

Modules will be the unit of interoperability with other programming languages.
* An OpenSCAD script is imported as a Module. Modules are the mechanism
  by which Curv can use existing OpenSCAD scripts. There's a limitation,
  because if the OpenSCAD script defines two entities with the same name
  (as variable, function or module), then those ambiguous entities are not
  accessible. (Maybe there's an option for mangling the names to make them
  unambiguous: eg, prepend the character `v`, `f` or `m` to each exported
  OpenSCAD identifier.)
* For C interoperability, we could use LLVM to compile a Curv module
  into a shared object or DLL, plus a header file,
  and we could import a properly constructed shared object or DLL
  as a Curv module.

## Curv Modules

Modules are used to represent reusable libraries and parameterized shapes.

This is part of a simplified alternative to the Object proposal,
in which "simple objects" are replaced by Records
and "scoped objects" are replaced by Modules.

A module constructor is either a script file,
or a sequence of one or more `;` terminated statements
surrounded by `{}` brackets.
Within a module constructor, definitions whose names begin with `_`
are private, and are not exported.
Some definitions may be marked as parameters using the `param` keyword.

This proposal is a simplification of OpenSCAD2's Object proposal,
because it splits Objects into two separate concepts, Records and Modules,
and it specializes the design and restricts the operations on each concept
for ease of understanding and implementation.

Operations on modules:
* record operations -- a module contains a set of name/value pairs,
  and when operated on using record operations, it behaves like it is
  implicitly converted to a record.
* list operations -- a module contains an ordered sequence of values,
  and when used with a list operation, it behaves like it is implicitly
  converted to a list, discarding the fields.
* function call -- a module has zero or more named parameters.
* `import(filename)` -- map \*.curv and \*.scad files to a module value.
* `use module;` -- argument must be static; add public fields to scope

There is no concept of module inheritance, which avoids a major
area of complexity from the OpenSCAD2 proposal. There are no `include`
or `overlay` operations. Nothing in this design prevents this from being
added later if the need arises.

However, the function call notation for customizing a module still
supports the idea of prototype-oriented programming.

## Static Expressions
An expression is static if all of its free variables are static:
* Builtin bindings like `true` and `sqrt` are static.
* A function formal parameter is not static.
* A `let` bound variable is static if its definiens is static.
* A parameter binding of a module is not static.
* A non-parameter binding of a module is static if its definiens is static.
* In the case of recursive definitions within a `let` or module,
  we tentatively assume that all non-parameter bindings are static,
  then attempt to disprove this by finding non-static free variables
  in the definientia. If no such non-static free variables are found,
  then the binding is considered static.

Staticness is a property of expressions which is computed by the
semantic analyzer. Its only role in the language is within the `use` operator:
the argument must be a static expression.

Within the compiler, staticness is relevant to optimization and performance.
Static expressions are compile time constants, and are computed exactly once.

A more liberal definition of staticness, that made more expressions static,
would be possible, at the expense of more complex language rules.
For example, we could say that `if(B)C;else A` is static if B is static,
and B is true, and C is static (disregarding whether A is static).
But this is a descent into endless complexity, so I'll avoid that for now.

## The `use` operator
`use ModuleExpr;` requires ModuleExpr to be a compile time constant
during the name resolution phase.

This is tricky, because Russell's Paradox could occur:
```
use script("foo.curv");
use script("bar.curv");
```
What happens if either module exports `script`? Or, for that matter, `use`?

To avoid paradoxes,
* `use` is a keyword operator, not a builtin function that can be shadowed.
* The argument to `use` is resolved in the surrounding scope.
  In effect, we prevent Russell's paradox by introducing logical levels.

As a result, this is illegal:
```
foo = script("foo.curv");
use foo;
```

## Type Predicates and Equality
Decision:
* The set of all values is partitioned by the type predicates
  for the 8 basic types.
* is_module(M) is true for a module.
* is_record(M), is_list(M) and is_fun(M) are false for a module.
* Modules are considered to be distinct from the 6 light weight
  data types: null, bool, num, string, list and record, which support
  meaningful equality comparisons.
* All modules compare equal, and compare unequal to members of the 7
  other basic types. (By contrast, equality is meaningful for records.)

Maybe Module is a subtype of Record, List and Function.
If M is a module, then is_record(M), is_list(M) and is_fun(M) are all true.

Okay, but now I'm not sure how equality works,
in the context of M==List or M==Record.
* Equality gets weird if you have subtypes that add additional information
  that's not present in the supertype.
* The "correct" approach that is consistent with subtyping and Reynold's Law
  is to have a different equality operator for each type.
  The supertype equality test identifies values that are considered different
  by the subtype equality test.
  This approach is common in statically typed languages.
* A universal equality operator doesn't obey Reynold's Law
  if subtypes which add additional information are also present.
* Caml has objects, subtypes, and object equality which tests for
  "structural equality". If two objects are equal, then they really are equal,
  but if they are unequal, then they still might be considered equal with
  respect to a supertype that they both belong to.

I think this is not a big deal: equality is not intended to be useful
for modules. But, I still have to define what == means for modules.
* If is_fun(M) is true for all modules, and if all functions are equal
  to each other, then does this mean that functions and modules are equal
  to each other? If not, then you can construct an is_module predicate
  using is_fun and ==.
* Maybe all modules are equal to each other, and unequal to other kinds
  of values, following the rationale for function equality.
* You can make multiple clones of a given module,
  with different parameter values, using function call notation.
  Equality might be meaningful for comparing clones, to test if they have the
  same parameter values. Whether this is useful, I dunno.

If universal equality and this particular notion of subtypes are not
compatible, then maybe the subtype relations need to go.
* Does is_list really make sense, given that list is a simple data type,
  and modules are not simple data, and not intended to be compared
  using equality? Should the is_vec3 predicate ever be true for a module?
  How useful is it to `reverse` a module? How would you implement `reverse`,
  given the requirement to preserve dependencies if the reserved module
  is later customized?
