The Core Language
=================

This document is the reference manual and design rationale
for the Curv core language, which unlies the shape library.

Design Goals
------------
The Curv language is designed for two groups of users:
artists, who use Curv to create geometric shapes,
and library developers, who extend the shape library
with new geometric primitives.

The language design goals encompass both groups of users:

* Ease of use.
* Expressive power.
* Rendering speed.
* Interoperability with other programming languages.
* Safety and security.

Language Characteristics
------------------------

* Simple, elegant, powerful.
* Dynamically typed.
* Pure functional language. No I/O. Programs are expressions.
* Curried functions, patterns, list/record/string comprehensions.
* Array language: scalar arithmetic operations work on vectors, matrices and higher dimensional arrays.
* 7 data types: the 6 JSON types, plus function values.
* Superset of JSON. Curv is a data interchange format for pure functional data.
