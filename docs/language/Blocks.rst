Blocks
======

A block is a statement or expression with local variables.

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
