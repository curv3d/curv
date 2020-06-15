Records
-------
A record is a set of fields (name/value pairs).
If ``R`` is the record value ``{a:1,b:2}``,
then ``R.a`` is ``1``, the value of field ``a``.

Records are used to represent:

* sets of labeled arguments in a function call;
* geometric shapes (each field is a shape attribute);
* libraries.

Record Constructors
~~~~~~~~~~~~~~~~~~~
A record constructor is an expression that constructs a record value.
It is surrounded by brace brackets ``{...}``.

There are two kinds of record constructor:

1. Record comprehensions. E.g., ``{a:1, b:2}``.
2. Modules (scoped record constructors). E.g., ``{a=1; b=2;}``.

Unlike modules, record comprehensions do not introduce a scope (field specifiers can't refer to one another).
This means you can write a record comprehension that references local variables
with the same names as the field names, like this: ``{x:x,y:y}``.
This makes record comprehensions the preferred syntax for specifying labeled arguments
in a function call.  For example::

    rotate {angle: 45*deg, axis: Z_axis} cube

Record comprehensions permit you to override fields in existing records
or specify default values. Using the ... operator you can add one or more
fields to a record. And using if statements and or loops you can conditionally
add fields.

Modules (scoped record constructors) contain a set of definitions
that form a mutually recursive scope. Definitions can refer to one another,
and you can define recursive functions. Modules are used to define libraries.
See ``lib/curv/*.curv`` in the source tree for examples.
Note that these files begin and end with ``{`` and ``}``.

Record Comprehensions
~~~~~~~~~~~~~~~~~~~~~
A record comprehension consists of a comma- or semicolon-separated list of field generators
inside of brace brackets. A field generator is a kind of `statement`_ that adds 0 or more
fields to the record being constructed.

A simple field generator constructs exactly one field:

* *identifier* ``:`` *expression*
* *quoted_string* ``:`` *expression*

For example,

* ``{a: 1, b: 2}``
* ``{"a": 1, "b": 2}`` -- Same as above.

The spread operator (``...``) interpolates all of the fields
from one record into the record being constructed.

* ``...`` *record_expression*

Field generators are processed left to right. If the same field name is
specified more than once, then the last occurrence wins.
This can be used to specify defaults and overrides:

* ``{x: 0, ... r}`` -- The same as record ``r``, except that if field ``x`` is
  not defined, then it defaults to ``0``.
* ``{... r, x: 0}`` -- The same as record ``r``, extended with field ``x``,
  with ``x:0`` overriding any previous binding.

Complex field generators may be composed from simpler ones
using if statements, for loops, and other control structures, as described in `Statements`_.
For example,

* ``{for (i in 1..3) "f$i" : i}``
  evaluates to ``{f1: 1, f2: 2, f3: 3}``.

.. _`statement`: Statements.rst
.. _`Statements`: Statements.rst

Modules
~~~~~~~
Modules are an alternate syntax for constructing record values, and are useful when
one field definition must reference another.

A module is a semicolon-separated or semicolon-terminated list of definitions
inside of brace brackets. The definitions form a single, mutually recursive scope,
meaning that definitions can refer to one another, and you can define recursive functions.
The order of the definitions doesn't matter. Duplicate definitions are an error.
For definition syntax, see: `Blocks`_.

.. _`Blocks`: Blocks.rst

Record Operations
~~~~~~~~~~~~~~~~~
``record . identifier``
  The value of the field named by ``identifier``.
  Eg, ``r.foo``.

``record . quotedString``
  The value of the field named by ``quotedString``.
  Eg, ``r."foo"``.
  The field name need not be a constant. Eg, ``r."$x"``.

``defined (record . identifier)``
  True if a field named ``identifier`` is defined by ``record``, otherwise false.

``defined (record . quotedString)``
  True if a field named ``quotedString`` is defined by ``record``, otherwise false.

``fields record``
  The field names defined by ``record`` (as a list of strings).

``is_record value``
  True if the value is a record, false otherwise.

``merge listOfRecords``
  Merge all of the fields defined by the records in ``listOfRecords``
  into a single record. If the same field is defined more than once,
  the last occurrence of the field wins.
  Same as::

    {for (r in listOfRecords) ...r}
