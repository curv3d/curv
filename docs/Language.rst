=================
The Curv Language
=================

Curv is a domain specific language for specifying geometric objects.
It's a language for artists, and it prioritizes simplicity and exploratory
programming over the complex features provided by general purpose programming languages
to support large scale software engineering.

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
record             ``{name: "Lancelot", quest: "To seek the holy grail", favourite_colour: "blue"}``
function           ``x -> x+1``
==============     ============================================

The first 6 data types are taken from JSON,
and Curv can be considered a superset of JSON.

There are no named types or user-defined types.
All application data is represented using the 7 data types.
For example, a geometric point or vector is a list of numbers.
A matrix is a list of lists of numbers. A geometric shape is a record
containing 5 fields (two booleans, two functions and a matrix).

All values are immutable (there are no mutable objects).
All data structures are hierarchical and finite, and therefore printable.
Curv data structures cannot contain cycles, and thus cannot be infinite,
with the exception that functions can be recursive.
These restrictions make Curv programs easier to understand and debug.

Null
----
``null`` is a placeholder value that, by convention, indicates that
some situation does not hold, and that the corresponding value is not available.
Its distinguishing characteristic
is that it is different from any other value. The only available
operation is testing a value to see if it is null: ``value==null``
is true if the value is null, otherwise false.

Boolean
-------
The Boolean values are ``true`` and ``false``.
They are used for making decisions:
if some condition holds, do this, otherwise do that.

The relational operators compare two values and return a boolean:

==============     ============================================
``a == b``         ``a`` is equal to ``b``
``a != b``         ``a`` is not equal to ``b``
``m < n``          ``m`` is less than ``n``
``m <= n``         ``m`` is less than or equal to ``n``
``m > n``          ``m`` is greater than ``n``
``m >= n``         ``m`` is greater than or equal to ``n``
==============     ============================================

(Note: ``a`` and ``b`` are arbitrary values, ``m`` and ``n`` are numbers.)

The ``==`` operator is an equivalence relation:

* For every value ``a``, ``a == a``.
* For any pair of values ``a`` and ``b``, if ``a==b`` then ``b==a``.
* For any three values ``a``, ``b`` and ``c``, if ``a==b`` and ``b==c`` then ``a==c``.

The logical operators take boolean values as arguments, and return a boolean:

==========   =============================================================
``a && b``   Logical and: True if ``a`` and ``b`` are both true.
``a || b``   Logical or: True if at least one of ``a`` and ``b`` are true.
``!a``       Logical not: ``!true==false`` and ``!false==true``.
==========   =============================================================

The conditional operator selects between two alternatives based on a boolean condition::

  if (condition) result_if_true else result_if_false


Number
------
String
------
List
----
Record
------
Function
--------

Program Structure
=================
Curv programs do not have side effects. The only goal of a Curv program is
to compute a value: usually this is a geometric shape, but it can also be a
library. (Shapes and libraries are both represented by record values.)

Conventional programming languages are "statement languages". A statement
is a syntactic form that either defines a named object (a definition)
or causes a side effect (an action). In such a language, a program is a sequence
of statements, the body of a function is a sequence of statements (one of which
is a "return statement"), and expressions occur only as elements of statements.

Curv is an "expression language". An expression is a syntactic form that
computes a value, eg like ``2+2``. A program is an expression. The body of a function
is an expression. Statements only occur embedded in certain expression forms.
(Note that JSON is also an expression language, and that most JSON programs are
also legal Curv programs.)
