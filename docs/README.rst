1. The User Interface
=====================

The user interface is the ``curv`` program. After building and installing ``curv``, use ``curv --help`` for command line options.

There is no language documentation, but you can look at ``examples/*.curv`` and ``lib/std.curv`` to see working curv code.

Try this::

  $ curv examples/shreks_donut.curv

Try this::

  $ curv
  curv> 2+2
  4
  curv> cube 1

2. The Core Language
====================
Curv is a *domain specific language* for constructing 2D and 3D
geometric shapes. It's not a general purpose programming language.
There is no concept of I/O. A Curv program is simply an expression that
computes a shape, without causing side effects.

Curv is *dynamically typed*, with 8 types. The first 6 data types are
borrowed from JSON: null, boolean, number, string, list and record.
In addition, we have functions and shapes. There are no user defined types,
and Curv is not "object oriented".

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

  sphere 1 >> colour red >> translate(10,0,0)

A *block* allows locally scoped definitions to be included in an expression::

  ( definition1; definition2; ...; result_expression )
  
Here are examples of definition syntax::

  pi = 3.141592653589793;
  incr x = x + 1;
  add(a,b) = a + b;

Within a block, all definitions are recursive, and the scope of each definition
is the entire block. The order of definitions does not matter.

Curv is an *expression language*: programs are expressions, blocks are expressions,
``if (cond) a else b`` is an expression, etc. Consequently, every syntactic construct
can be nested in every other construct.

..
  Curv programs are stored in ``*.curv`` files.
  A Curv program is an expression that computes a value.
  A typical Curv program computes a shape

3. The High Level Geometry Interface
====================================
The "high level geometry interface" is a set of primitive shapes,
and a set of operations for transforming and combining shapes to create
new shapes.

The goal is for this interface to be both powerful and easy to use.

* It's powerful because there is a rich collection of powerful operations
  that can be combined in many different ways.
* Building new shapes is as easy as plugging together existing shapes and
  operators like Lego.

This interface is under development. There's no documentation,
but you can read the code in `<../lib/std.curv>`_ and read the comments
to see the interface. You can look in `<../examples>`_ to see examples.

4. The Low Level Geometry Interface
===================================
The "low level geometry interface" allows you to create new primitive
shapes and operations by directly specifying the signed distance field
of the resulting shape. The primary interface is the ``make_shape`` function,
but there are also low level library functions that work on distance fields,
and low level language features used to generate good GPU code.

This interface needs a ton of work, and will be the last part of the language to be
properly documented. You can see the low level interface at work in the
implementation of the high level geometry interface: see `<../lib/std.curv>`_.

To understand the theory behind the low level interface,
here are some resources that I am using:

* Seminal academic paper: `Sphere Tracing`_
* Inigo Quilez helped make this popular: `Modelling with Distance Functions`_
* Video: `How to Create Content with Signed Distance Functions`_

.. _`Sphere Tracing`: http://graphics.cs.illinois.edu/sites/default/files/zeno.pdf
.. _`Modelling with Distance Functions`: http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
.. _`How to Create Content with Signed Distance Functions`: https://www.youtube.com/watch?v=s8nFqwOho-s
