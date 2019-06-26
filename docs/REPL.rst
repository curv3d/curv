The REPL: An interactive command line interpreter
=================================================
If you invoke the ``curv`` command without any arguments,
then you will get an interactive command line interpreter,
called the `REPL`_.

.. _`REPL`: https://en.wikipedia.org/wiki/Read%E2%80%93eval%E2%80%93print_loop

The Command Language
--------------------
Each command that you type is interpreted as a Curv program.
In the REPL dialect of Curv, any syntactic phrase can be used as a command:
you can execute expressions, definitions and statements.
This makes the REPL a good environment for exploring the Curv language:
you can type any phrase and see what happens.

An `expression`_ is evaluated, yielding a value, which is printed.
::

  curv> 2 + 2
  4

.. _`expression`: language/Expressions.rst

shape

A `definition`_ binds one or more names to values. The bindings are added
to the REPL environment, and can be used in later commands.
::

  curv> x = 17
  curv> x
  17

.. _`definition`: language/Blocks.rst

mutually recursive

A `statement`_ is executed for its side effects, and may optionally print
some results or output. There are three cases:
actions, value generators and field generators.

.. _`statement`: language/Statements.rst

actions

value generators

field generators

Special Variables
-----------------
``_`` is bound to the value of the most recent expression.
