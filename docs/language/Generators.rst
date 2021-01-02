Generators
==========
A **generator** is a generalized expression that produces a sequence
of zero or more values.

For example, the generator ``1,2,3``, when evaluated, produces
the values ``1 2 3``. You can try this in the REPL::

    curv> 1, 2, 3
    1
    2
    3

Generators are part of the syntax of `list constructors`_ and
`dynamic record constructors`_.
For example, the list constructor ``[1,2,3]`` consists of two parts:

* The brackets ``[ ]`` indicate we are constructing a list.
* The generator ``1,2,3`` produces the list elements.

.. _`list constructors`: Lists.rst
.. _`dynamic record constructors`: Records.rst

Generator Syntax
----------------
Generators are declarative programs for specifying a sequence of values.
The syntax for generators includes local variables, sequencing,
conditionals and loops.

expression
    An expression is a generator that produces a single value.

g1, g2, g3
    A sequence of generators separated by commas will evaluate
    each generator in left-to-right order. You can optionally
    append a trailing comma, which is ignored.

the empty generator
    The empty generator has no tokens, and produces no values.
    It is found between the brackets of the empty list constructor ``[]``.
    You can visualize the empty generator by parenthesizing it: ``()``.

``(`` *generator* ``)``
    Any generator can be parenthesized without changing its meaning.

``...`` *collection*
    The **spread operator** (``...``) takes a list or record as argument.
    It produces each element of the list, or each field of the record.
    Fields are produced as [symbol,value] pairs.

if (condition) generator
    If the *condition* is true, then evaluate the *generator*
    and produce its values. Otherwise, don't produce any values.

if (condition) g1 else g2
    If the *condition* is true, then evaluate *g1*,
    otherwise evaluate *g2*.

for (name in list) generator
    Iterate over the elements of *list*.
    For each element value, bind it to *name* as a local variable
    within *generator*, and evaluate *generator*.

for (name in list until stop_condition) generator
    Iterate over the elements of *list*,
    stopping early if *stop_condition* becomes true.
    For each element value, bind it to *name* as a local variable
    within *generator*, and evaluate *generator*.

let <definitions> in <generator>
    Bind the <definitions> as local variables within <generator>.

Example
-------
Here is a list constructor that yields the list ``[4,16,36,64,100]``,
by computing the even squares between 1 and 100::

    [
        for (x in 1..10)
            let n = x*x in
                if (n `mod` 2 == 0)
                    n
    ]

In Haskell, Python, and a few other languages, you would accomplish the
same thing using a *list comprehension*. Curv's generator syntax provides
a superset of the capabilities of list comprehensions in these languages.
Here's a syntax comparison:

=======  ===========================================================
Haskell  ``[n | x <- [1..10], let n = x*x, n `mod` 2 == 0]``
Python   ``[n for x in range(1,11) for n in (x*x,) if n % 2 == 0]``
Curv     ``[for (x in 1..10) let n = x*x in if (n `mod` 2 == 0) n]``
=======  ===========================================================

Imperative Generators
---------------------
You can use the imperative statement language to write generators.
This gives you the machinery of mutable local variables, assignments,
and while loops.

The syntax is: you write a compound statement (imperative
statements separated by semicolons), and you insert generators
*as statements* into the imperative program.

This example produces ``[1,2,3,4,5]``::

    [
        local i = 1;
        while (i <= 5) (
            i;
            i := i + 1;
        );
    ]

The statement ``i;`` is an expression generator that produces each
list element.
