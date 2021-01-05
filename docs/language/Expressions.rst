Expressions
===========

An expression is a phrase (syntactic unit) which denotes a value.
For example, ``2+2`` is an expression which denotes the value *4*.
When using the shape library, ``cube`` is an expression which denotes a
geometric shape (the cube enclosing the unit sphere).

Curv expressions do not have side effects†. The only outcome of evaluating
an expression is to produce a value, or to abort the program with an error
if the result is undefined. (For example, ``0/0`` is a numeric expression
that aborts the program with an error.
For more about errors, see: `Design by Contract`_.)

  † Curv has a debugger, and language features for interacting with the
  debugger. If you are using these features, then trace messages can be
  written to the debug console during expression evaluation. However, these
  "side effects" are not considered part of the language semantics.
  See: `Statements`_.

.. _`Design by Contract`: Design_by_Contract.rst
.. _`Statements`: Statements.rst

Curv is an expression-oriented language.
Curv programs are constructed primarily from nested expressions.
This contrasts with conventional imperative programming languages,
which are statement-oriented.

* A Curv program is an expression. A program is run by evaluating this
  expression, which yields a value, which is usually a geometric shape.
  Curv programs do not have side effects.
  This contrasts with imperative languages,
  where a program is a list of statements, which are executed for their
  side effects.
* The body of a Curv function is an expression, which is evaluated to
  yield the result of a function call. Functions do not have side effects.
  This contrasts with imperative languages,
  where the body of a function is a list of statements,
  and a ``return`` statement is executed to specify a result.
* Most importantly, geometric shapes are values, and are constructed
  by evaluating expressions. Shape values can be arbitrarily combined
  or transformed by applying shape operations, which yield new shape values.
  This contrasts with imperative 3D modelling languages,
  where shapes are constructed by executing a list of statements, which
  modify a global canvas or scene graph, and where global variables (such as
  "the current transformation matrix") are used to modify the effect of
  operations. Curv's expression-oriented programming style is cleaner, easier
  to use, and more powerful.
