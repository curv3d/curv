Orthogonality
=============
"Entities must not be multiplied beyond necessity" -- Occam's Razor

"Programming languages should be designed not by piling feature on top of feature,
but by removing the weaknesses and restrictions that make additional features appear necessary.
Scheme demonstrates that a very small number of rules for forming expressions,
with no restrictions on how they are composed,
suffice to form a practical and efficient programming language
that is flexible enough to support most of the major programming paradigms in use today."
-- Scheme R3RS, 1986

"Most programming languages are partly a way of
expressing things in terms of other things and partly a
basic set of given things. The ISWIM (If you See What I
Mean) system is a byproduct of an attempt to disentangle
these two aspects in some current languages."
-- The Next 700 Programming Languages, P.J. Landin, 1966

So the idea here is...

* Orthogonality
* The opposite of orthogonality is having multiple competing features that do the same job,
  but with different restrictions. You choose one of the competing features for a particular
  task, then you live with those particular restrictions. As opposed to removing the restrictions
  and unifying the competing features into a single feature.
* And then these orthogonal features, stripped of their restrictions, can be composed together
  in a larger number of ways, which makes the language richer and more expressive.

Only One Kind of Null
---------------------
Javascript has ``undefined``, ``null`` and ``NaN``.
Curv just has ``null``.

Only One Kind of Boolean
------------------------
In Curv, ``true`` is the only value that counts as the boolean truth value in a logical operation,
and ``false`` is the only value that counts as the boolean false value.

Consequently, the laws of boolean algebra work. In particular, the && and || operators are commutative.

In most dynamically typed languages, there are multiple values that count as true,
and multiple values that count as false, but there is no general agreement on which values should be
true and which values should be false.

In these languages, the laws of boolean algebra are broken.  && and || are not commutative.

Only One Kind of Number
-----------------------
In Curv, there is only one kind of number. All numbers are represented as IEEE 64 bit floats.

An integer is just a number whose fractional part is 0. ``1`` and ``1.0`` are the same number.

In most other programming languages, there are at least two kinds of numbers, integers and reals.
So ``1`` and ``1.0`` are different values, and they are treated differently by many fundamental operations.

Only One Kind of List
---------------------
In Curv, a "list" is an ordered sequence of values. There is only one such data type.

Most languages have multiple data types which fill this role.

In Curv, the function call operator is orthogonal from the list forming syntax.
Function call applies a function to a single argument. f(x,y) applies f to a single argument,
(x,y), which happens to be a list. This, in turn, simplifies "monoid" functions such as "max"...

Only One Kind of Record
-----------------------
records and objects and modules are unified into a single concept

Only One Kind of Function
-------------------------
OpenSCAD has functions and modules.

C++ has functions, template classes, template functions.

Only One Kind of Name
---------------------
In Curv, there is only a single namespace, for naming all the things.

* Python has one namespace for variables, functions and classes, and a separate module namespace.
* Haskell has separate namespaces for types and variables.
* OpenSCAD has 3 separate namespaces for variables, functions and modules.
