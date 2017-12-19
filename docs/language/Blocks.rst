Blocks
======

A block is a statement or expression with local variables.

For example,

::

  let a = 1;
      b = 2;
  in a + b

is an expression with two local variables ``a`` and ``b``. The result is ``5``.

A "local variable" is just an immutable association between a name and a value.
Within the scope of a variable definition like ``a=1``,
variable names have substitution semantics. Any reference to ``a``
can be replaced by its value ``1`` without changing the meaning of the program.

The order in which variables are defined within a block doesn't matter.
Function definitions may be recursive or mutually recursive.

Block Syntax
------------
Blocks are created using the ``let`` and ``where`` operators:

  | ``let`` *definitions* ``in`` *phrase*
  | *phrase* ``where`` *definitions*
  | ``let`` *definitions* ``in`` *phrase* ``where`` *more_definitions*

where *phrase* is either a statement or an expression,
and *definitions* is a sequence of definitions, separated by semicolons,
with an optional terminating semicolon.
In the 3rd syntax, *definitions* and *more_definitions* are combined into a single scope.

These syntaxes are equivalent: they give you a stylistic choice
whether to put the phrase before, after, or in the middle of the definitions.

Definition Syntax
-----------------
Basic definition syntax:

 | *identifier* ``=`` *expression*
