The Imperative Sublanguage
==========================
Curv has two imperative features:

``name := value``:
  An assignment statement that allows you to modify a
  local variable defined using ``let``, ``where`` or ``for``.
``while (condition) statement``:
  A "while" statement.

These features allow you to write code in an imperative style.
The semantics of these features are restricted,
so that it is impossible to define impure functions.
Curv retains its pure functional semantics.
For a detailed discussion, see: `<../advanced/Imperative.rst>`_.

These features exist for 3 reasons:

* Makes it easier to port code from an imperative language.
* It's an aid to users whose primary programming experience
  is with imperative languages, and who are not fully fluent
  in the functional programming style.
* This is the only way to iterate within a shape's distance function.
  The Geometry Compiler does not yet support tail recursion.
  (Due to the limitations of GLSL, this is a complex feature that is not currently planned for release 1.0.)
