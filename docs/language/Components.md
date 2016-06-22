# Components
A "component" is:
* A "software component": a unit of reusable software, with a contractually
  specified interface, with explicit dependencies on other components,
  that can be deployed independently, and is subject to composition
  by third parties.
* A set of mutually recursive definitions, plus a sequence of values.
* A reification of a script as a first class value.

The word "module" would be better, but that's already taken.
I might rename it anyway, because this concept is a close match to
* javascript modules
* python modules
* ML modules

An OpenSCAD script is imported as a Component. Components are the mechanism
by which Curv can use existing OpenSCAD scripts. There's a limitation,
because if the OpenSCAD script defines two entities with the same name
(as variable, function or module), then those ambiguous entities are not
accessible.

Components are used to represent reusable libraries and parameterized shapes.

This is part of a simplified alternative to the Object proposal,
in which "simple objects" are replaced by Records
and "scoped objects" are replaced by Components.

A component constructor is either a script file,
or a sequence of one or more `;` terminated statements
surrounded by `{}` brackets.
Within a component constructor, definitions whose names begin with `_`
are private, and are not exported.
Some definitions may be marked as parameters using the `param` keyword.

This proposal is a simplification of OpenSCAD2's Object proposal,
because it splits Objects into two separate concepts, Records and Components,
and it specializes the design and restricts the operations on each concept
for ease of understanding and implementation.

Operations on components:
* record operations -- a component contains a set of name/value pairs,
  and when operated on using record operations, it behaves like it is
  implicitly converted to a record.
* list operations -- a component contains an ordered sequence of values,
  and when used with a list operation, it behaves like it is implicitly
  converted to a list, discarding the fields.
* function call -- a component has zero or more named parameters.
* `import(filename)` -- map \*.curv and \*.scad files to a component value.
* `use component;` -- argument must be static; add public fields to scope

There is no concept of component inheritance, which avoids a major
area of complexity from the OpenSCAD2 proposal. There are no `include`
or `overlay` operations. Nothing in this design prevents this from being
added later if the need arises.

However, the function call notation for customizing a component still
supports the idea of prototype-oriented programming.

## Type Predicates and Equality
Decision:
* The set of all values is partitioned by the type predicates
  for the 8 basic types.
* is_comp(C) is true for a component.
* is_rec(C), is_list(C) and is_fun(C) are false for a component.
* Components are considered to be distinct from the 6 light weight
  data types: null, bool, num, string, list and record, which support
  meaningful equality comparisons.
* All components compare equal, and compare unequal to members of the 7
  other basic types. (By contrast, equality is meaningful for records.)

Maybe Component is a subtype of Record, List and Function.
If C is a component, then is_record(C), is_list(C) and is_fun(C) are all true.

Okay, but now I'm not sure how equality works,
in the context of C==List or C==Record.
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
for components. But, I still have to define what == means for components.
* If is_fun(C) is true for all components, and if all functions are equal
  to each other, then does this mean that functions and components are equal
  to each other? If not, then you can construct an is_component predicate
  using is_fun and ==.
* Maybe all components are equal to each other, and unequal to other kinds
  of values, following the rationale for function equality.
* You can make multiple clones of a given component,
  with different parameter values, using function call notation.
  Equality might be meaningful for comparing clones, to test if they have the
  same parameter values. Whether this is useful, I dunno.

If universal equality and this particular notion of subtypes are not
compatible, then maybe the subtype relations need to go.
* Does is_list really make sense, given that list is a simple data type,
  and components are not simple data, and not intended to be compared
  using equality? Should the is_vec3 predicate ever be true for a component?
  How useful is it to `reverse` a component? How would you implement `reverse`,
  given the requirement to preserve dependencies if the reserved component
  is later customized?
