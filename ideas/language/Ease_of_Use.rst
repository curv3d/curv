Ease of Use in a Pure Functional Language
=========================================

Curv is a language for procedurally generated 3D art. The language is
designed for two sets of users. First, it is a live programming environment
for programmer/artists, which means that it needs to be simple, high level, and
easy to use. Second, Curv is its own extension language: all of its graphical
primitives are written in Curv, and Curv code can be compiled directly into
GPU shader programs. This means that Curv must be powerful enough to satisfy
the needs of expert graphics programmers, and support GPU compilation.

The best way to satisfy these requirements is with a referentially transparent,
pure functional language, as opposed to an imperative, object-oriented
language.  The popular graphics languages that demonstrate the greatest ease
of use are value-oriented and functional: OpenSCAD, and the many box & wire
visual languages used by graphical artists. At the low level, programming
with shared mutable state is incompatible with GPU programming.

Haskell is the most well-known pure functional language. Haskell has a steep
learning curve, mostly due to its extremely complex static type system, but
also due to the need to learn complex and abstract idioms, such as tail
recursion and monads, in order to write even simple programs. One contribution
of this paper is to demonstrate a pure functional language that focuses on
ease of use for beginners.

While building Curv as a pure functional language, we discovered practical
reasons to support imperative programming idioms such as mutable variables
and while loops. At the high level, new users of Curv are most likely to be
familiar with imperative programming idioms from languages like Python and
Javascript. They will be more comfortable writing iterative code using mutable
variables and while loops, as opposed to functional-style tail recursion.
At the low level, it is easier to port graphical algorithms written in other
programming languages if you don't have to transform iteration into tail
recursion.

Curv is a referentially transparent, pure functional language, which
paradoxically supports imperative programming idioms such as mutable variables
and while loops. At first glance, Curv would appear to be a hybrid imperative
/ functional programming language, and many such languages exist. But all of
those other hybrid languages are imperative at their core: they have shared
mutable state, functions are not pure, and expressions are not referentially
transparent. Functional programming is just an add-on. By contrast, Curv
has a pure functional core. The second contribution of this paper is to show
how to support imperative idioms while maintaining referential transparency.

--------------
This is based on a number of considerations.

Haskell showed that pure functional
programming is practical for general purpose programming. Popular languages

This is based on the author's previous experience with Haskell and OpenSCAD.
---

To satisfy these requirements, Curv is a referentially transparent,
pure functional language, which paradoxically supports imperative
programming idioms such as mutable variables and while loops.
Based on this statement, Curv would appear to be a hybrid imperative /
functional programming language, and many such languages exist. But all of
those other hybrid languages are imperative at their core: they have
shared mutable state, functions are not pure, expressions are not
referentially transparent. Functional programming is just an add-on.
By contrast, Curv has a pure functional core, and the imperative features
are just add-on syntax which do not violate the referential transparency
of expressions.

This paper will explore why Curv requires this dual nature,
the syntax of Curv, and how referential transparency is preserved in the
presence of imperative features.

1. Requirements
---------------
Curv has a tiered API.
The high level graphics API is simple and easy to use for programmer / artists.
The low level graphics API is more difficult, but it provides expressive power
for experts, and allows the high level API to be written entirely in Curv.

### The High Level API
The high level API is based on Constructive Solid Geometry (CSG).

### The Low Level API

2. Syntax
---------

3. Semantics
------------
