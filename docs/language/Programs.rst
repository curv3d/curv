Programs
========
A Curv program is an expression. When you run a program, you evaluate this
expression, and the result is a value.

A Curv program can be structured as one or more ``*.curv`` source files.

Programs are Expressions
------------------------
Since programs are expressions,
the `"Hello, World!"`_ program would look like this::

  "Hello, World!"

This is an expression which evaluates to the string ``"Hello, World!"``.

.. _`"Hello, World!"`: https://en.wikipedia.org/wiki/%22Hello,_World!%22_program

But Curv is a language for making geometric shapes. A more realistic example
of a first program is::

  cube

which evaluates to a cube, which will be displayed in the graphics window.

Here is a larger example, which constructs a lollipop shape::

  let
  diam = 1;
  len = 4;

  in let
  candy = sphere diam >> colour red;
  stick = cylinder{h: len, d: diam/8} >> move[0,0,-len/2];

  in
  union[candy, stick]

In order to keep the model parameters separated from the auxiliary
definitions, I've placed them in separate (nested) ``let`` clauses.

Note that:

* ``sphere diam`` is a function call.
  It calls the ``sphere`` function with the argument ``diam``,
  and the result is a sphere.
* ``cylinder{h: len, d: diam/8}`` is another function call.
  It calls the ``cylinder`` function with labelled arguments.
* ``>>`` is the pipeline operator.
  It is often used to send a shape through a series of geometric
  transformations, with data flowing left to right, similar
  to a Unix shell pipeline.
* See: `Functions`_.

.. _`Blocks`: Blocks.rst
.. _`Functions`: Functions.rst

Source Files
------------
If the previous example is in the file ``lollipop.curv``,
then you can reference it from another program like this::

  let
  lolly = file "lollipop.curv";
  
  in
  row .4 [lolly, lolly, lolly]

``file filename``
  Evaluate the program stored in the file named ``filename``,
  and return the resulting value. ``filename`` is a string.

Libraries
---------
If your program is so large that you need to split it up into
multiple source files, then you'll probably have a main source file
that computes the shape, and some auxiliary source files that contain
model parameters and auxiliary definitions.

A set of definitions is represented by a record value.
When we store a set of definitions in a source file, we call this a library.

Here is how the lollipop example could be split into 3 source files:

``lollipop.curv``::

  let
      include file "lolli-lib.curv"
  in
  union[candy, stick]

``lolli-parameters.curv``::

  {
  diam = 1;
  len = 4;
  }

``lolli-lib.curv``::

  {
  include file "lolli-parameters.curv";
  candy = sphere diam >> colour red;
  stick = cylinder{h: len, d: diam/8} >> move[0,0,-len/2];
  }

Note that:

* A set of definitions, surrounded by brace brackets, is called a module,
  and evaluates to a record value. See: `Records`_.
* ``include record_value`` is a special kind of definition that adds all
  of the fields in a record to the current scope.
  See: `Blocks`_.

.. _`Records`: Records.rst
.. _`Blocks`: Blocks.rst
