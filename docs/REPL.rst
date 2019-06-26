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
This makes the REPL a good environment for learning the Curv language
or testing a program:
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

Assignment actions will modify any variable that was previously
defined in this REPL session using a definition. However, you cannot
assign a variable defined by the standard library.
::
  curv> x = 1       // definition
  curv> x := x + 1  // assignment action
  curv> x
  2

The standard debug and assertion actions are available.
::
  curv> print "Hello world"
  Hello world

A `value generator`_ is normally found in a list constructor;
it is a kind of statement that adds zero or more elements to the list being constructed.
You can execute a value constructor directly from the REPL::
  curv> for (i in 1..5) i^2
  1
  4
  9
  16
  25

.. _`value generator`: language/Lists.rst

A `field generator`_ is normally found in a record constructor;
it is a kind of statement that adds zero or more fields to the record being constructed.
You can execute a field constructor directly from the REPL::
  curv> foo:17; bar:42
  foo:17
  bar:42

.. _`field generator`: language/Records.rst

Special Variables
-----------------
``_`` is bound to the value of the most recent expression.
