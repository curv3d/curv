Symbols and Variants
====================

Symbols
-------
Symbols are simple, abstract values with no internal structure.
``#foo`` is a symbol; it prints as ``#foo``; it is only equal to itself.

Symbols are used when you need to distinguish between a fixed set
of named alternatives. For example,

* In Curv, the `Boolean values`_ are ``#true`` and ``#false``.
* When exporting a .X3D mesh file, you use the ``-Ocolouring=`` command line
  option to specify whether you want face colouring or vertex colouring.
  These two alternatives are represented by symbols, so the command line
  syntax is ``-Ocolouring=#face`` or ``-Ocolouring=#vertex``.

.. _`Boolean values`: Boolean_Values.rst

By convention, the symbol ``#null`` can be used as the null value.
When exporting Curv data as JSON, ``#null`` is converted to JSON ``null``.

Symbols are used internally to represent field names in records.
The ``fields`` function takes a record as an argument, and returns a list
of symbols.

Variants
--------
Variant types are a tool for organizing and modelling data.
A variant type consists of a fixed set of alternatives.
Each alternative has a name, and optionally a value.
To construct an instance of a variant type,
you specify the name of one of the alternatives,
plus a value, if that alternative carries a value.

Variant types are a feature of modern statically typed programming languages,
where they have many names: discriminated unions, enumerated types,
variant types, and algebraic data types.

In Curv, you don't need to declare a variant type before you can construct
a variant value. Instead, variants are constructed directly:

* A variant with just a name is a symbol. Eg, ``#foo``.
* A variant with a name and a value is a record with a single field.
  Eg, ``{bar: x}``.

For example, Curv represents value picker configuration internally as variants:

* ``{slider:[low,high]}``
* ``{int_slider:[low,high]}``
* ``#scale_picker``
* ``#checkbox``
* ``#colour_picker``

If one of your alternatives means "do nothing" or "the value is not available",
then by convention, you can use the symbol ``#null`` for that.

Variant constructors are also patterns.
You can use pattern matching and the ``match`` operator to process variant
data. For example::

  match [
    {slider:[low,high]} -> ...;
    {int_slider:[low,high]} -> ...;
    #scale_picker -> ...;
    #checkbox -> ...;
    #colour_picker -> ...;
  ]

Symbol Names
------------
A symbol name may contain any printable characters (including spaces), but
not control characters (tab and newline are forbidden).
In the general case, symbol names are quoted with apostrophes, like this::

    #'hello world!'

Within a symbol name, an apostrophe character must be written as
a 2-character escape sequence::

    '_

For example::

    #'This isn'_t a plain identifier.'

But if the name matches the syntax of an plain Curv identifier,
consisting only of alphanumeric characters and underscores, and with
a non-numeric first character, then the quotes may be omitted, like this::

    #hello_world

A symbol name may be empty: the empty symbol prints as ``#''``.

The Symbol API
--------------
``is_symbol x``
    A predicate that returns ``#true`` if ``x`` is a symbol.

``symbol str``
    Convert a string argument to a symbol.
    As a special case, consider that ``symbol "true"`` and ``symbol "false"``
    construct the boolean values.

Use ``string`` to convert a symbol to a string.
