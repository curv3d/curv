Symbols and Variants
====================

Symbols
-------
Symbols are simple, abstract values with no internal structure.
``#foo`` is a symbol; it prints as ``#foo``; it is only equal to itself.

Symbols are used when you need to distinguish between a fixed set
of named alternatives. For example,

* In Curv, the boolean values are ``#true`` and ``#false``.
* When exporting a .X3D mesh file, you use the ``-Ocolour=`` command line
  option to specify whether you want face colouring or vertex colouring.
  These two alternatives are represented by symbols, so the command line
  syntax is ``-Ocolour=#face`` or ``-Ocolour=#vertex``.

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

Symbol API
----------
Symbol names may contain spaces and punctuation characters other than ``_``.
The syntax is ``#'hello world!'``.
In other words, you precede a quoted identifier with a ``#`` to construct a quoted symbol.
The use case for this will arise once symbol names appear as labels in the value picker GUI.

The function ``is_symbol x`` is a predicate that returns ``#true`` if ``x` is a symbol.
