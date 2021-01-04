Definitions
===========
A definition is a phrase that binds zero or more names to values,
within a lexical scope.

* For example, the data definition ``zero = 0``
  binds the name ``zero`` to the value ``0``.
* For example, the function definition ``succ n = n + 1``
  binds the name ``succ`` to the function value ``n->n+1``.

1. Definitions are used to define local variables within an expression,
statement or generator. For example, the expression::

  let zero = 0;
      succ n = n + 1;
  in succ zero

has two local variables ``zero`` and ``succ``. The result is ``1``.

2. Definitions are used to specify the members of a *module*
(aka a scoped record constructor). For example, the module::

  {
    zero = 0;
    succ n = n + 1;
  }

is a record with two members ``zero`` and ``succ``.

When one or more definitions appear together in a ``let`` phrase
or in a module, they form a *recursive scope*. Definitions may refer
to one another, and the order in which they are written does not matter.
You can define recursive functions which call themselves.

3. Definitions are part of the syntax of *local definitions*,
which are statements of the form ``local <definition>``, and which define
mutable local variables within the scope of a compound statement.
Local definitions have *sequential scope*, not recursive scope.
The scope of the bound variables begins at the statement following the
local definition, and continues to the end of the compound statement.

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

*definition1* ``,`` *definition2* ``,`` ...
  Two or more definitions separated by commas, with an optional trailing comma,
  are a compound definition. The meaning of a compound definition is:
  bind all of the names mentioned in all of the definitions.
  The order in which the definitions are written doesn't matter.
  If the same name is bound twice, report an error.

*definition1* ``;`` *definition2* ``;`` ...
  Two or more definitions separated by semicolons, with an optional trailing
  semicolon, are a compound definition. There is no difference between
  comma and semicolon, except style and esthetics.

``(`` *definition* ``)``
  A definition may be enclosed in parentheses without changing its meaning.

Let Phrases
-----------
A *let phrase* has the syntax::

    let <definition> in <body>

where *definition* is frequently a compound definition,
and *body* is an expression, generator or statement.

The semantics are: define local variables that are visible within *body*.

.. Undocumented feature, required in Curv 0.4, but I don't like using it
.. in Curv 0.5:
.. If the body is a statement or a ``do`` expression, then the body may
.. contain assignment statements that modify the values of variables during
.. statement execution. See: `Statements`_.

.. If let-bound variables are immutable, then we can say:
.. Variable names have substitution semantics. Any reference to ``a`` can be
.. replaced by its value ``1`` without changing the meaning of the program.

.. Blocks
.. ======
.. A block is a statement or expression with local variables.
.. 
.. A block contains a set of definitions,
.. plus a body, which is a statement or expression.
.. For example,
.. 
.. ::
.. 
..   let a = 1;
..       b = 2;
..   in a + b
.. 
.. is an expression with two local variables ``a`` and ``b``. The result is ``3``.
.. 
.. If a block body is a statement or a ``do`` expression, then the body may
.. contain assignment statements that modify the values of variables during
.. statement execution. See: `Statements`_.
.. 
.. Otherwise, the block body is an expression, and variables are immutable.
.. In that case, within the scope of a variable definition like ``a=1``,
.. variable names have substitution semantics. Any reference to ``a``
.. can be replaced by its value ``1`` without changing the meaning of the program.
.. 
.. The scope of each definition is the entire block, which includes all the definitions.
.. Function definitions may be recursive or mutually recursive.
.. The ordering of definitions within a block doesn't matter.

.. _`Functions`: Functions.rst
.. _`Patterns`: Patterns.rst
.. _`Statements`: Statements.rst
.. _`Design by Contract`: Design_by_Contract.rst
