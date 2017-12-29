The Imperative Sublanguage
=============================
Curv contains an "imperative sublanguage", implemented by the ``do`` operator,
which allows you to define mutable variables using ``var``,
reassign those variables using ``:=``, and iterate using
a ``while`` action. This allows you to write code in an imperative style.
The semantics of this feature are restricted, so that it is impossible to define
impure functions. Curv retains its pure functional semantics.

This feature exists for 3 reasons:

* Makes it easier to port code from an imperative language.
* It's an aid to users whose primary programming experience
  is with imperative languages, and who are not fully fluent
  in the functional programming style.
* In the 0.0 release, this is the only way to iterate within a shape's distance
  function. The GPU compiler is not yet smart enough to convert tail recursion
  into iteration.

This feature is experimental, and may change in future.
