# Atoms: Identifier Objects

I need a representation of identifiers, during analysis and runtime,
in the following contexts:
* function parameters
  * the list of parameter names within a function value
  * the list of argument labels within a function call
  * need an algorithm that compares these two lists and constructs
    the argument list
* module field names
  * during analysis, a `Module_Expr` accumulates a symbol table
  * a module value contains a dictionary mapping names to slot ids
* record field names
  * a record is a map from names to values
  * need to look up a name (efficiently)
  * need to merge two records
  * pattern matching
* a `Dot_Expr` has a field name which must be looked up
  in a record value or module value

## A Global Ordering of Atoms
There are many contexts where, abstractly, I need to merge two maps from
identitiers onto entities (values, expressions, slot indexes); sometimes
the two maps have different entity types. If there is a single global ordering
of identifiers, then I can have an O(N) merge of two maps of different type,
running map iterators in parallel. Or use the same data structure everywhere.

It would be nice if this global ordering were persistent across sessions.
That would allow the ordering to be used in files that cache compilation
or evaluation results. Also, deterministic unit tests.

Choices of global ordering:
* Alphabetical ordering. weight balanced binary tree (used by Haskell data.map)
  or binary search array or std::map. Atom is a String.
* Hash ordering. hash tree (clojure and erlang). Switch to alphabetic ordering
  on hash collision. Atom maybe contains hash code followed by string data.
  Order function is `</==` on hash code first, then string data.
  * A possible optimization is to use global atom table, and use pointer
    equality for `==`. Still examine the atom data for `<`.
* Keep a global atom table in the session. Use pointer ordering.
  Requires a session object, and ordering not consistent across sessions.

I could hide this choice behind an abstraction.
* `Atom` type with `<` and `==`. Can be used with `Atom_Map`, `std::map`,
  binary search array.
* `Atom_Map` container template. iterator, get, set, merge.
* `Session` required to convert string to `Atom`.

## Minimum Viable Product
The simplest choice is alphabetic ordering.

Here is a good-enough first-draft MVP representation:
* function value: list of parameter names, in parameter order
* function call: list of argument labels, unordered
* `Module_Expr` symbol table is a map of atoms to exprs
* Module dictionary is a sorted array of atoms, or a map from atom to slotid
* a record value is a map of names to values, with a merge operation,
  eg weight balanced binary tree or hashmap.
* a record expression is a record value with values replaced by expressions.
  Evaluation is a deep copy of the tree, replacing exprs with values.

call resolution only requires atom equality:
```
argc is the # of parameters in the function we are calling.
make an array of arg values/arg expressions of size argc.
initialize array with the 'missing' value.
add the positional arguments to the arg array.
add the labeled arguments to the arg array:
  for (argl : arg_labels)
    lookup argl in parameter_list (linear search)
    if not found, raise error.
    if arg array[parameter index] not missing, raise error.
    add argument to arg array.
scan the arg array looking for missing arguments:
  if found, fill in the default value, or raise error if no default
```

Record merge. Records represented by a tree with refcounted nodes.
Weight balanced binary tree (haskell:data.map) or hashtree (clojure, erlang).

Record pattern match. Need to: match up pattern names with keys, detect
missing keys, detect excess keys. Similar to requirements for merge: traverse
both sets of keys in parallel.
