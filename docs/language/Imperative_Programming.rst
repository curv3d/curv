Imperative Programming
======================
Even though Curv is a pure functional language,
you can write imperative style code using assignment statements
and standard imperative control structures:
compound statements, if statements, for statements and while statements.

Curv has two imperative features:

``name := value``
  An assignment statement that allows you to modify a
  local variable defined in an enclosing scope
  using ``let``, ``where`` or ``for``.
``while (condition) statement``
  A "while" loop. The condition tests one or more variables
  which are modified by assignments within the loop body on each iteration.

These features are combined with other imperative control structures:
see `<Statements.rst>`_.
You can write imperative code inside of `do`
expressions, and in list, record and string comprehensions.

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
