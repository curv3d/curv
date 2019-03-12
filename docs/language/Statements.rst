Statements
==========
Curv supports imperative style programming by providing statements,
which are executed in sequence for their side effects.
Statements encapsulate the idea of sequential evaluation in Curv.

Simple statements, like the ``print`` statement and the assignment statement,
are combined and sequenced using control structures:
compound statements, ``if`` statements, ``for`` statements and ``while`` statements.
Local variables are defined over statement scope using ``let`` clauses.

Rationale:

* Statements are an aid to new users whose previous programming experience
  is with imperative languages. Basic imperative idioms like conditional assignment
  and while loops can be used in Curv.
* Statements make it easier to port code from an imperative language.
  You don't need to rewrite imperative algorithms in a functional style
  (eg, converting while loops to tail recursion) before you can get the code running.
* ``For`` and ``while`` loops are the only way to iterate within a shape's distance function.
  The Geometry Compiler does not yet support tail recursion.
  (Due to the limitations of GPU shader languages,
  this is a complex feature that is not currently planned for release 1.0.)

Assignment statements are not usually found in pure functional languages.
Curv restricts the semantics of mutable variables and assignment statements
so that it retains its pure functional semantics.
For a detailed discussion, see: `<../advanced/Imperative.rst>`_.

Using Statements
----------------
Curv is an expression-oriented language:
programs, function bodies, and function calls are expressions, not statements.
You use statements by embedding them in an expression, such as a ``do`` expression.

``do statements in expression``
  Execute the statements in sequence, then evaluate the expression and return its result.

For example, Curv programs can be debugged by inserting ``print`` statements
and other debug actions. You can attach debug actions to an expression using a ``do`` clause::

  f x =
      do print "calling function f with argument x=$x"
      in x + 1;

Curv has an assignment statement, which lets you write algorithms
in an imperative style.

``name := expression``
  An assignment statement modifies a local variable
  defined in an enclosing scope using ``let``, ``where`` or ``for``.

For example, here is a function that sums a list, written in imperative
style using an assignment statement::

  sum a =
      let total = 0
      in do
          for (e in a)
              total := total + e;
      in total;

A list comprehension is just a list constructor (``[a,b,c]``)
that contains a sequence of statements, instead of just a sequence of expressions.
This allows you to use an imperative algorithm for constructing the elements of a list.

Here is a Haskell list comprehension that yields ``[4,16,36,64,100]``
(the even squares between 1 and 100)::

  [n | x <- [1..10], let n = x*x, n `mod` 2 == 0]

In Curv, this is written using statement syntax::

  [for (x in 1..10) let n = x*x in if (mod(n, 2) == 0) n]

Statements are similarly used in record and string comprehensions.

Statement Types
---------------
There are 3 kinds of statements:

Element generators (found in list constructors)
  An element generator generates a sequence of 0 or more values,
  which are added to the list under construction.
  See: `Lists`_.
Field generators (found in record constructors)
  A field generator generates a sequence of 0 or more fields,
  which are added to the record under construction.
  See: `Records`_.
Actions
  An action is a generic statement that can be used in
  any statement context. Actions are the only statements
  allowed in ``do`` expressions.
  
  * A debug action has side effects which aid in debugging:
    printing a message on the debug console, or terminating the
    program in case of error.
    See: `Debug Actions`_.
  * An assignment statement: ``name := value``.

Statement Operators
-------------------
Curv has a set of generic operations for constructing complex statements
out of simpler statements of the same type.
This includes all of the standard imperative control structures.

Parenthesized statement: ``(statement)``
  Any statement can be wrapped in parentheses without changing its meaning.

Compound statement: ``statement1; statement2; ...``
  Two or more statements of the same type, separated by semicolons, are executed in sequence.
  An optional terminating semicolon may be added.

Single-arm conditional: ``if (condition) statement``
  The statement is only executed if the condition is true.
  See: `Boolean Values`_.

Double-arm conditional: ``if (condition) statement1 else statement2``
  Execute statement1 if the condition is true, otherwise execute statement2.
  Both statements have the same type.
  See: `Boolean Values`_.

Bounded iteration: ``for (pattern in list_expression) statement``
  The statement is executed once for each element in the list.
  At each iteration,
  the element is bound to zero or more local variables by the pattern.
  See: `Patterns`_.

Unbounded iteration: ``while (condition) statement``
  The statement is executed repeatedly, zero or more times,
  until ``condition`` becomes false. The condition tests one or
  more variables which are modified by assignments within
  the loop body on each iteration.

Local variables: ``let definitions in statement``
  Define local variables over the statement. See: `Blocks`_.

Local variables: ``statement where definitions``
  An alternate syntax for defining local variables. See: `Blocks`_.

Chained statements: ``do statements in statement2``
  First execute the statements, then execute statement2.
  This is a variant syntax, equivalent to a compound statement.
  It is useful in the context of ``do statements in let definitions in statement``
  since it avoids adding a trailing parenthesis.

.. _`Boolean Values`: Boolean_Values.rst
.. _`Lists`: Lists.rst
.. _`Records`: Records.rst
.. _`Debug Actions`: Debug_Actions.rst
.. _`Blocks`: Blocks.rst
.. _`Patterns`: Patterns.rst
