Records
-------
A record is a set of fields (name/value pairs).
If ``R`` is the record value ``{a:1,b:2}``,
then ``R.a`` is ``1``, the value of field ``a``.

Records are used to represent:

* sets of labeled arguments in a function call;
* geometric shapes (each field is a shape attribute);
* modules or libraries.

Record Constructors
~~~~~~~~~~~~~~~~~~~

A record constructor consists of a comma-separated list of field generators
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
using blocks and control structures, as described in `Statements`_.
For example,

* ``{for (i in 1..3) "f$i" : i}``
  evaluates to ``{f1: 1, f2: 2, f3: 3}``.

.. _`statement`: Statements.rst
.. _`Statements`: Statements.rst

Modules
~~~~~~~

A module is a semicolon-separated or semicolon-terminated list of definitions
inside of brace brackets. The definitions form a single, mutually recursive scope.
The order of the definitions doesn't matter. Duplicate definitions are an error.
For definition syntax, see: `Blocks`_.

.. _`Blocks`: Blocks.rst

Modules are an alternate syntax for constructing record values, and are useful when
one field definition must reference another.

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

``merge listOfRecords``
  Merge all of the fields defined by the records in ``listOfRecords``
  into a single record. If the same field is defined more than once,
  the last occurrence of the field wins.
  Same as::

    {for (r in listOfRecords) ...r}
