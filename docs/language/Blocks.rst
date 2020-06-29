Blocks
======

A block is a statement or expression with local variables.

A block contains a set of definitions,
plus a body, which is a statement or expression.
For example,

::

  let a = 1;
      b = 2;
  in a + b

is an expression with two local variables ``a`` and ``b``. The result is ``3``.

If a block body is a statement or a ``do`` expression, then the body may
contain assignment statements that modify the values of variables during
statement execution. See: `Statements`_.

Otherwise, the block body is an expression, and variables are immutable.
In that case, within the scope of a variable definition like ``a=1``,
variable names have substitution semantics. Any reference to ``a``
can be replaced by its value ``1`` without changing the meaning of the program.

The scope of each definition is the entire block, which includes all the definitions.
Function definitions may be recursive or mutually recursive.
The ordering of definitions within a block doesn't matter.

Block Syntax
------------
Blocks are created by attaching ``let``\ ...\ ``in`` and ``where``\ ... clauses to a phrase:

  | ``let`` *definitions* ``in`` *phrase*
  | *phrase* ``where`` *definitions*
  | ``let`` *definitions* ``in`` *phrase* ``where`` ``(`` *more_definitions* ``)``

where *phrase* is either a statement or an expression,
and *definitions* is a sequence of definitions, separated by semicolons,
with an optional terminating semicolon.
In the 3rd syntax, *definitions* and *more_definitions* are combined into a single scope.

These syntaxes are equivalent: they give you a stylistic choice
whether to put the phrase before, after, or in the middle of the definitions.

Definition Syntax
-----------------
*identifier* ``=`` *expression*
  The fundamental definition syntax.
  The value of *expression* is bound to *identifier*.

*pattern* ``=`` *expression*
  The *pattern* is matched against the value of *expression*.
  If the pattern match succeeds, identifiers within the pattern
  are bound to either the entire value, or components of the value.
  If the pattern match fails, the program aborts with an error.
  For example,
  
  ::
  
  [x,y] = normalize p
  
  evaluates ``normalize p``, which is required to yield a 2-element list.
  The first and second elements of this list are bound to ``x`` and ``y``.
  See: `Patterns`_.

*identifier* *pattern* ``=`` *expression*
  A function definition, which is just an abbreviation of
  
   | *identifier* ``=`` *pattern* ``->`` *expression*
  
  For example,
  
  ::
  
    plus[x,y] = x + y
  
  defines a function ``plus`` whose argument is a 2-element list.
  This is an abbreviation of::
  
    plus = [x,y] -> x + y
  
  See: `Functions`_.

*identifier* *pattern1* *pattern2* ... ``=`` *expression*
  A curried function definition, which is an abbreviation of
  
   | *identifier* ``=`` *pattern1* ``->`` *pattern2* ``->`` ... ``->`` *expression*
  
  See: `Functions`_.

``include`` *record*
  Add all of the fields in *record* to the current scope.
  *record* is an expression that is evaluated at compile time,
  and may reference only built-in operations.

``test`` *statement*
  Execute the statement for its side effects.
  Do not define any variables.
  A test definition is normally used for embedding unit tests in a module.
  See `Design by Contract`_.

.. _`Functions`: Functions.rst
.. _`Patterns`: Patterns.rst
.. _`Statements`: Statements.rst
.. _`Design by Contract`: Design_by_Contract.rst
