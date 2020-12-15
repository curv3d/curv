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

Haskell and Python invent new syntax for list comprehensions,
while Curv reuses the existing statement syntax.

See `Imperative`_ for more information on statement syntax.

.. _`Imperative`: Imperative.rst

Extended List Comprehensions
----------------------------
Here are some additional details that go beyond the existing Haskell/Python
model.

A generator statement has one of the following forms:

<expression>
    Add a single value to the list being constructed.

<expr1>, <expr2>, <expr3>
    Add multiple values to the list being constructed.
    This must be parenthesized if passed as an argument to a control
    structure like ``if`` or ``for``, since ``,`` has very low operator
    precedence.

``...`` <list-expression>
    Add all the elements of the specified list to the list being constructed.

``...`` <record-expression>
    Add all the fields of the specified record to the list being constructed.
    Each field is converted to a ``[fieldname,element]`` pair.
    Field names are symbol values.
    The pairs are generated in alphabetical order by field name.

You've just learned that ``1,2,3`` is a generator statement.
If you place this statement in a list constructor, you get ``[1,2,3]``.
(So it turns out that regular list constructor syntax is a special case
of list comprehension syntax.)
If you type this statement into the REPL, each generated value
is printed on a separate line::

    curv> 1, 2, 3
    1
    2
    3

You can debug list comprehensions in the REPL by typing component statements
and seeing what values they generate.

Imperative List Comprehensions
------------------------------
List comprehensions are normally considered to be a declarative syntax,
but because Curv reuses its statement syntax, you have full access to
imperative programming, including local variables, assignment statements,
and while loops. This example produces ``[1,2,3,4,5]``::

    [
        local i = 1;
        while (i <= 5) (
            i;
            i := i + 1;
        );
    ]

The statement ``i;`` is a generator statement.

The reason this works is composability: every Curv language construct can
be composed with every other language construct, without restriction.
So we don't have a special corner of the language where
``for`` loops work but ``while`` loops are forbidden.

Record Comprehensions
---------------------
coming soon
