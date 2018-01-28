1. The User Interface
=====================

The user interface is the ``curv`` program. After building and installing ``curv``, use ``curv --help`` for command line options.

Batch mode::

  $ curv examples/shreks_donut.curv
  **a graphics window opens, displaying the shape**
  
Interactive CLI mode::

  $ curv
  curv> 2+2
  4
  curv> cube
  **a graphics window opens, displaying the shape**
  curv>

Live Programming Mode:

* Open a text editor window, editing ``myshape.curv``.
* Open a terminal window and run ``curv -l myshape.curv`` from ``bash``.
* Each time you save changes to ``myshape.curv``, the file will be re-evaluated
  and the new shape will be displayed in a graphics window.
* Keep the terminal window visible: if there are errors in your Curv program,
  they will be displayed here.

2. The Core Language
====================
Full language documentation: `<language/README.rst>`_.

Curv is a *domain specific language* for constructing 2D and 3D
geometric shapes. It's not a general purpose programming language.
There is no concept of I/O. A Curv program is simply an expression that
computes a shape, without causing side effects.

Curv is *dynamically typed*, with 7 types. The first 6 data types are
borrowed from JSON: null, boolean, number, string, list and record.
In addition, we have functions.
All application level data is built from these 7 types.
For example, a geometric shape is simply a record value
with a standard set of fields.
There are no mechanisms for naming or defining types.

Curv is an *array language*: scalar arithmetic operations are generalized
to work on vectors, matrices, and higher dimensional arrays. A vector is
represented as a list of numbers: the syntax is ``(x,y,z)`` or ``[x,y,z]``.
A matrix is a list of vectors.

Curv is a *pure functional language*. This means that functions are values,
and that functions are "pure": they compute a result value that varies only
based on the argument values. There is no global mutable state
that a function can read.

Function call (``f x``) is a binary operation that applies the function ``f``
to the argument ``x``. There are 3 ways to pass more than one argument
to a function:

Argument lists:
  ``f(x,y)`` can be thought of in two ways: as passing a single argument
  (a 2-list) to the function ``f``, or as passing 2 arguments.
Argument records:
  ``rotate{angle: a, axis: v}`` is a function call where the argument is a
  record value, which can be interpreted as a function call with multiple
  named arguments.
Curried functions:
  ``f x y`` is a function call ``f x``, returning another function that is
  applied to ``y``. This can be interpreted as a call to a "Curried" function
  with two arguments.

A nested function call like ``f(g(h x))``
can be rewritten as ``x >> h >> g >> f``, which reads like a Unix pipeline,
with data passing from left to right. When combined with curried functions,
this syntax is used for chaining together geometry operations without
nested parentheses. For example::

  sphere 1 >> colour red >> move(10,0,0)

A *let block* allows locally scoped definitions to be included in an expression::

  let definition1; definition2; ... in result_expression
  
Here are examples of definition syntax::

  pi = 3.141592653589793;
  shape = cube >> colour red;
  factorial n = product(1..n);

Within a let block, the scope of each definition is the entire block,
and function definitions may be recursive or mutually recursive.
The order of definitions does not matter.

Curv is an *expression language*: programs are expressions, blocks are expressions,
``if (cond) a else b`` is an expression, etc.
Consequently, every syntactic construct can be nested in every other construct.

..
  Curv programs are stored in ``*.curv`` files.
  A Curv program is an expression that computes a value.
  A typical Curv program computes a shape

3. The Shape Library
====================
Full shape library documentation: `<shapes/README.rst>`_.

The "shape library" is a set of primitive shapes,
and a set of operations for transforming and combining shapes to create
new shapes.

The goal is for this interface to be both powerful and easy to use.

* It's powerful because there is a rich collection of powerful operations
  that can be combined in many different ways.
* Building new shapes is as easy as plugging together existing shapes and
  operators like Lego.

You can look in `<../examples>`_ to see examples.

The low level interface used to implement the shape library is
poorly documented right now.
You can read the source code in `<../lib/std.curv>`_.
See `<Theory.rst>`_ more more information about how the shape library works.
