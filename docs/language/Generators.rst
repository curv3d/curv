Generators (List and Record Constructors)
=========================================
A list constructor such as ``[1,2]`` consists of two parts:

* The brackets ``[ ]`` indicate we are constructing a list.
* The generator ``1,2`` indicates the list elements.

A **generator** is a phrase that produces a sequence of values.
The example ``1,2`` produces two values.

Generators have an independent meaning, outside of the list constructor syntax.
You can execute a generator in the REPL::

    curv> 1,2
    1
    2

The syntax for generator phrases includes sequencing, conditional, and loop
operators that take generators as arguments. Generators can use the full
power of the statement language.

Record constructors like ``{a:1,b:2}`` work the same way.

* ``a:1,b:2`` is a generator that produces two fields.
* ``a:1`` is a field expression, which is a standalone phrase that you can
  execute in the REPL.

Expressions
-----------
An expression is a generator that produces exactly one value.

* For example, ``2`` is an expression, and ``[2]`` is a list containing
  one element.
* For example, ``a:2`` is a field expression,
  and ``{a:2}`` is a record containing one field named ``a``.

*Note*: A field expression like ``a:2`` actually evaluates to
a symbol/value pair, which in this case is ``[#a,2]``.
So ``{[#a,2]}`` is just an ugly, less readable synonym for ``{a:2}``.
This fact is useful in case you need to compute a field name at runtime
using a symbol expression. For example::

    {[symbol("foo"++"bar"), 42]}

evaluates to ``{foobar: 42}``.

Sequences
---------
A comma-separated sequence of zero or more generators
is a compound generator that runs each constituent generator in sequence.
Try typing each of the following generators into the REPL:

+----------------+-----------------------+
| This generator | produces these values |
+----------------+-----------------------+
|                |                       |
+----------------+-----------------------+
| ``1``          | ``1``                 |
+----------------+-----------------------+
| ``1,2``        | ``1  2``              |
+----------------+-----------------------+
| ``1,2,3``      | ``1  2  3``           |
+----------------+-----------------------+

If there is at least one item, then you can include an optional trailing comma.
If we enclose each of the above generators in square brackets,
then we get the standard syntax for list constructors::

    []
    [1]      [1,]
    [1,2]    [1,2,]
    [1,2,3]  [1,2,3,]

*Note*: Any generator can be parenthesized without changing its meaning.
So ``()`` is the empty generator, and ``(a,b)`` generates first *a*, then *b*.
This becomes important later, when we introduce conditional and loop operators
that take generators as arguments.

For example, if we nest one sequence in another,
then we discover that the following generators are equivalent::

    1, (2, 3)
    (1, 2), 3
    1, 2, 3

The Spread Operator
-------------------
The **spread operator** (``...``) produces all of the items in its argument,
which is a list or record. Try it in the REPL::

    curv> ...[1,2]
    1
    2
    curv> ...{a:1,b:2}
    a:1
    b:2

The spread operator allows you to interpolate all of the items in a list
or record into another list or record.

In a record, fields are processed left to right. If the same field name is
specified more than once, then the last occurrence wins.
This can be used to specify defaults and overrides:

* ``{x: 0, ... r}`` -- The same as record ``r``, except that if field ``x`` is
  not defined, then it defaults to ``0``.
* ``{... r, x: 0}`` -- The same as record ``r``, extended with field ``x``,
  with ``x:0`` overriding any previous binding.

List Comprehensions
-------------------
Some programming languages (eg, Python, Haskell) provide
**list comprehensions**, which are short declarative programs
that generate the elements of a list, written in a special compact syntax.

In Curv, there is no special "list comprehension" syntax,
but statement operators like ``if``, ``let`` and ``for``
can be applied to generators. You can use the following syntax
to simulate list comprehensions:

* ``if (condition) generator`` is a conditional generator.
* ``let <definitions> in generator`` defines local variables
  within a generator.
* ``for (<variable> in <list>) generator`` runs a generator in a loop,
  once for each element of a list.

Here is a list constructor that yields the list ``[4,16,36,64,100]``,
by computing the even squares between 1 and 100::

    [
        for (x in 1..10)
            let n = x*x in
                if (n `mod` 2 == 0)
                    n
    ]

Here's a comparison of the same list comprehension written in several languages.

=======  ===========================================================
Haskell  ``[n | x <- [1..10], let n = x*x, n `mod` 2 == 0]``
Python   ``[n for x in range(1,11) for n in (x*x,) if n % 2 == 0]``
Curv     ``[for (x in 1..10) let n = x*x in if (n `mod` 2 == 0) n]``
=======  ===========================================================

Statements
----------
You can use the entire statement language to write generators.
A generator can be used in any context where a statement is legal.
You have full access to imperative programming, including mutable local
variables, assignment statements, and while loops.
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

The reason this works is composability: every Curv language construct can
be composed with every other language construct, without restriction.
