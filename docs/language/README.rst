The Core Language
=================

This document is the reference manual and design rationale
for the Curv core language, which unlies the shape library.

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

* Simple, elegant, powerful.
* Dynamically typed.
* Pure functional language. No I/O. Programs are expressions.
* Curried functions, patterns, list/record/string comprehensions.
* Array language: scalar arithmetic operations work on vectors, matrices and higher dimensional arrays.
* 7 data types: the 6 JSON types, plus function values.
* Superset of JSON. Curv is a data interchange format for pure functional data.
