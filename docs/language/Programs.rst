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
of your first program is::

  cube

which evaluates to a cube, which will be displayed in the graphics window.

Here is a larger example, which constructs a lollipop shape::

  let
  diam = 1;
  len = 4;

  in
  union(candy, stick)

  where
  candy = sphere diam >> colour red;
  stick = cylinder{h: len, d: diam/8} >> move(0,0,-len/2);

This illustrates a common coding pattern,
where you place model parameters at the top of the program
(using a ``let...in`` clause),
and auxiliary definitions at the bottom
(using a ``where...`` clause).
Both clauses are optional.

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

--------------------
Each source file is a *.curv file containing a Curv expression.
This expression is evaluated to yield a value.

A Curv program is typically stored in a source file
with the extension ``*.curv``.

Programs are expressions, which are evaluated to yield a result.

* In the common case, the result is a shape value.
* Another common pattern is to construct a record value,
  representing a set of definitions: either a library of
  reusable components, or a set of model parameters for a
  large, multi-source-file model.

Source Files and External Libraries
-----------------------------------
::

  file
  include record_expr
