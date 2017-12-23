Statements
==========

A statement is a syntactic unit (a phrase), distinct from an expression,
which expresses some action to carry out.
There are 3 kinds of statements:

* Element generators (found in list constructors).
  An element generator generates a sequence of 0 or more values,
  which are added to the list under construction.
  See: `Lists`_.
* Field generators (found in record constructors).
  A field generator generates a sequence of 0 or more fields,
  which are added to the record under construction.
  See: `Records`_.
* Debug actions.
  A debug action has side effects which aid in debugging:
  printing a message on the debug console, or terminating the
  program in case of error.
  See: `Debug Actions`_.

Curv has a set of generic operations for constructing complex statements
out of simpler statements of the same type.

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

Local variables: ``let definitions in statement``
  Define local variables over the statement. See: `Blocks`_.

Local variables: ``statement where definitions``
  An alternate syntax for defining local variables. See: `Blocks`_.

Local actions: ``do action in statement``
  First execute the action, then execute the statement.
  See: `Debug Actions`_ and `Imperative Sublanguage`_.

.. _`Boolean Values`: Boolean_Values.rst
.. _`Lists`: Lists.rst
.. _`Records`: Records.rst
.. _`Debug Actions`: Debug_Actions.rst
.. _`Imperative Sublanguage`: Imperative_Sublanguage.rst
.. _`Blocks`: Blocks.rst
.. _`Patterns`: Patterns.rst

