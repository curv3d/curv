# Nested Scopes

Refactor the definition compiler to support the following cases:

* (compound definition 1) where (compound definition 2)
  The unitary definitions from both compound defs are combined into a single
  dependency graph in a single Recursive_Scope object.

* {compound definition 1} where (compound definition 2)
  Ditto.
