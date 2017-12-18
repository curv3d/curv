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

A record constructor consists of a comma-separated list of field specifiers
inside of brace brackets. A field specifier has one of these forms:

* *identifier* ``:`` *expression*
* *quotedString* ``:`` *expression*
* ``...`` *recordExpression*

Field names can be arbitrary strings, but as an abbreviation, they can be
specified as just bare identifiers.
The spread operator (``...``) interpolates all of the fields from another
record into the record being constructed.

Field specifiers are processed left to right. If the same field name is
used more than once, then the last field specifier wins.

Examples:

* ``{a: 1, b: 2}``
* ``{"a": 1, "b": 2}`` -- Same as above.
* ``{x: 0, ... r}`` -- The same as record ``r``, except that if field ``x`` is
  not defined, then it defaults to ``0``.
* ``{... r, x: 0}`` -- The same as record ``r``, extended with field ``x``,
  with ``x:0`` overriding any previous binding.

Compound field specifiers may be constructed using blocks, and using the
``if``, ``for`` and ``;`` control structures, as described later.

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
