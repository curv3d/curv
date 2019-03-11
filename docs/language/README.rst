The Core Language
=================

This document is the reference manual and design rationale
for the Curv core language, which underlies the shape library.

Design Goals
------------
* Ease of use for artists and novice programmers.
  The focus is artistic exploration, not software engineering.
  No `boilerplate`_: the simple program ``cube`` is sufficient to construct a cube.
* Expressive power for expert programmers.
  The shape library is written entirely in Curv.
  Libraries of new shape operations can be written in Curv and distributed over the internet.
* Rendering speed.
  Curv programs are compiled into efficient GPU code for fast rendering.
* Interoperability with other programming languages.
  Two way data interchange with external programs. The shape library can be embedded
  in other programming languages (like Javascript and Python).
* Safety and security.
  A Curv program downloaded from the internet can't encrypt all your files
  or exfiltrate personal information to a server.

.. _`boilerplate`: https://en.wikipedia.org/wiki/Boilerplate_code

Language Characteristics
------------------------

Simple, Elegant, Powerful
  The Curv language is based on a small number of orthogonal concepts
  that can be combined together with no arbitrary restrictions.
  This results in a relatively simple language with a lot of expressive power.
  The core language is small enough to learn in a day.

Simple Type System
  Curv is a dynamically typed language with 7 value types:
  null, boolean values, numbers, lists, strings, records, and functions.
  There are no type names or type declarations.

Interoperability
  Curv is a superset of JSON. The type system comprises the 6 JSON data types,
  plus functions. Most JSON programs are also valid Curv programs.
  Since JSON is a standard data interchange format supported by all popular
  programming languages, this design provides three benefits:
  
  * Because the type system is so simple, it's easy to embed
    the Curv type system in another programming language.
    And that makes it feasible to import Curv data and libraries
    into other programming languages, or to export Curv data and libraries from
    other languages.
  * Curv can be used as a data interchange format for pure functional data.
  * Curv can import and export JSON data.

Pure Functional Language
  Curv is a pure functional language.
  
  * Functions_ are first class values, meaning that they can bound to variables,
    passed as function arguments and returned as function results.
  * Functions are pure, meaning that the result returned by a function depends
    only on the argument value, and not on shared mutable state, which doesn't
    exist in Curv.
  * Data structures are immutable values, not mutable objects.
  * A variable is an immutable association between a name and a value.
  * There is no I/O: Curv is not a scripting language.
    The only outcome of a running a program
    is to compute a value (which is usually a geometric shape).

Functional Programming Idioms
  Curried functions, pattern matching, list/record/string comprehensions.

Expression Language
  Curv is an expression language, not a statement language.
  A program is an expression, not a statement list.
  The body of a function is an expression, not a statement list.

Array Language
  Curv is an array language. Scalar arithmetic operations are generalized
  to work on Vectors_, Matrices_, and higher dimensional arrays called Tensors_.
  This makes geometric algorithms involving linear algebra easier to code,
  and these operations take advantage of vector hardware on GPUs for fast
  rendering.

Table of Contents
-----------------
Introduction: `Programs`_

The seven types of `Values`_:

* `Null`_
* `Boolean Values`_
* `Numbers`_ -- and `Trigonometry`_
* `Lists`_ -- which include `Vectors`_, `Complex Numbers`_,
  `Matrices`_, `Tensors`_
* `Strings`_
* `Records`_
* `Functions`_

Syntax:

* `Programs`_
* `Expressions`_
* `Blocks`_
* `Statements`_
* `Debug Actions`_
* `Patterns`_
* `Grammar`_
* `Imperative Programming`_

Design principles and patterns:

* `Design by Contract`_

.. _`Blocks`: Blocks.rst
.. _`Boolean Values`: Boolean_Values.rst
.. _`Complex Numbers`: Complex_Numbers.rst
.. _`Debug Actions`: Debug_Actions.rst
.. _`Design by Contract`: Design_by_Contract.rst
.. _`Expressions`: Expressions.rst
.. _`Functions`: Functions.rst
.. _`Grammar`: Grammar.rst
.. _`Imperative Programming`: Imperative_Programming.rst
.. _`Lists`: Lists.rst
.. _`Matrices`: Matrices.rst
.. _`Null`: Null.rst
.. _`Numbers`: Numbers.rst
.. _`Patterns`: Patterns.rst
.. _`Programs`: Programs.rst
.. _`Records`: Records.rst
.. _`Statements`: Statements.rst
.. _`Strings`: Strings.rst
.. _`Tensors`: Tensors.rst
.. _`Trigonometry`: Trigonometry.rst
.. _`Values`: Values.rst
.. _`Vectors`: Vectors.rst
