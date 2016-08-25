# Language Design Principles

Curv is a pure functional language.
It is a declarative language with simple semantics.

In some accounts, the difference between declarative and imperative languages
is explained by listing what's missing from declarative languages:
you can't assign a variable more than once,
there are no side effects.
This makes it seem that declarative languages are missing something, they
aren't as powerful as imperative languages.

However, that's not the case. This section provides a different kind of
explanation, by showing what properties Curv has that are missing from
imperative geometric description languages like Processing or OpenJSCAD.

## Pure Math
A Curv script is, basically, a pure mathematical description of a
geometric shape. Curv is essentially a mathematical notation.

In order for mathematics to be possible, a mathematical notation
needs to have some essential properties.

## The Law of Substitution
In Curv, there are no mutable variables or assignment statements.
There are, instead, defini
