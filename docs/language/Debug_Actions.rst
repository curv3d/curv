Debug Actions
-------------
Curv programs are debugged by inserting ``print`` statements and other debug actions.

Debug actions are statements, not expressions, but you can embed them
in expressions using a ``do`` expression.

``do actions in expression``
  Execute the ``actions``, then evaluate the ``expression`` and return its result.

  The ``action`` argument can be a simple action, as enumerated below,
  or it can be a compound action which is sequenced using ``if``, ``for`` and ``;``
  or which defines local variables using ``let`` and ``where``.
  For example, you can write a sequence of actions separated by ``;``,
  and they will be executed in sequence.

  The ``phrase`` argument can be an expression or statement.
  (A statement is an element generator in a list constructor,
  a field generator in a record constructor, or an action.)
  A ``do`` phrase can be used in any context where its ``phrase`` argument
  would also be legal.

  For example::

    f x =
      do print "calling function f with argument x=$x";
      in x+1;

  Then ``f 2`` returns ``3``, and as a side effect, prints a message
  to the debug console.

Simple Debug Actions
~~~~~~~~~~~~~~~~~~~~

``print message``
  Print a message string on the debug console, followed by newline.
  If ``message`` is not a string, it is converted to a string using ``repr``.

``warning message``
  Print a message string on the debug console, preceded by "WARNING: ",
  followed by newline and then a stack trace.
  If ``message`` is not a string, it is converted to a string using ``repr``.

``error message``
  On the debug console, print "ERROR: ", then the message string,
  then newline and a stack trace. Then terminate the program.
  If ``message`` is not a string, it is converted to a string using ``repr``.
  An error phrase is legal in either an action context, or in an expression context.
  Example of ``error`` in an expression context::
  
    if (x > 0) f(x) else error "bad x=$x"

``assert condition``
  Evaluate the condition, which must be true or false.
  If it is true, then nothing happens.
  If it is false, then an assertion failure error message is produced,
  followed by a stack trace, and the program is terminated.

``assert_error(error_message_string, expression)``
  Evaluate the expression argument.
  Assert that the expression evaluation terminates with an error,
  and that the resulting error message is equal to ``error_message_string``.
  Used for unit testing.

``exec expression``
  Evaluate the expression and then ignore the result.
  This is used for calling a function whose only purpose is to have a side effect
  (by executing debug actions) and you don't care about the result.
