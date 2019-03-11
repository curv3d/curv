Debug Actions
-------------
Curv programs are debugged by inserting ``print`` statements and other debug actions.

Debug actions are statements, not expressions, but you can attach them
to an expression using a ``do`` expression.

``do actions in expression``
  Execute the ``actions``, then evaluate the ``expression`` and return its result.

For example::

  f x =
      do print "calling function f with argument x=$x";
      in x+1;

Then ``f 2`` returns ``3``, and as a side effect, prints a message
to the debug console.

The *actions* in a ``do`` clause can be a simple debug action, as listed below,
or it can be an imperative style program written using debug actions,
compound statements, ``if`` statements, ``for`` statements, ``while`` statements,
``let`` statements (to introduce local variables) and assignment statements.
See: `<Statements.rst>`_.

For example, inside a ``do`` clause, you can write a sequence of actions separated by ``;``,
and they will be executed in sequence.

Debug actions can also be used as elements of list comprehensions, record comprehensions
and string comprehensions.

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
