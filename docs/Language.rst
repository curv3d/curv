=================
The Curv Language
=================

Curv is a dynamically typed pure functional language.
It has simpler semantics than any general purpose programming language.
It's powerful enough that the entire geometry API is written in Curv, not C++.
The core language is small enough to learn in a day.

Data Types
==========
Curv has 7 data types:

==============     ============================================
null               ``null``                
boolean            ``true, false``
number             ``-1, 0xFFEF, 3.1416``
string             ``"hello, world"``
list               ``[0,1,2]``
record             ``{name: "Doug", favourite_colour: "blue"}``
function           ``x->x+1``
==============     ============================================

The first 6 data types are taken from JSON,
and Curv can be considered a superset of JSON.

There are no named types or user-defined types.
All application data is represented using the 7 data types.
For example, a geometric point or vector is a list of numbers.
A matrix is a list of list of numbers. A geometric shape is a record
containing 5 fields (two booleans, two functions and a matrix).

All values are immutable (there are no mutable objects).
All data structures are hierarchical, and therefore printable:
it is impossible to form cycles in Curv data,
with the exception that functions can be recursive.
