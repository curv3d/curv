List and Record Comprehensions
==============================

List Comprehensions
-------------------
A **list comprehension** is a short declarative program that generates
the elements of a list, written in a compact syntax. Many programming
languages have this feature.

Here is a list comprehension that yields the list ``[4,16,36,64,100]``,
by computing the even squares between 1 and 100::

    [
        for (x in 1..10)
            let n = x*x in
                if (n `mod` 2 == 0)
                    n
    ]

In Curv, a list comprehension is just a list constructor (``[a,b,c]``)
that contains statements instead of expressions. In the above example,
we use a single statement, which is a ``for`` loop.

In a comprehension, we extend the statement language so that an
expression (which in this example is the final ``n``) is interpreted
as a statement that adds a value to the list being constructed.
(This is called a generator statement.)

Here's a comparison of the same list comprehension written in several languages.

=======  ===========================================================
Haskell  ``[n | x <- [1..10], let n = x*x, n `mod` 2 == 0]``
Python   ``[n for x in range(1,11) for n in (x*x,) if n % 2 == 0]``
Curv     ``[for (x in 1..10) let n = x*x in if (n `mod` 2 == 0) n]``
=======  ===========================================================

Haskell and Python invent new syntax for list comprehensions, while
in Curv, you just use the existing statement syntax.
