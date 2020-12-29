Records
-------
A record is a set of fields (name/value pairs).
If ``R`` is the record value ``{a:1,b:2}``,
then ``R.a`` is ``1``, the value of field ``a``.

Records are used to represent:

* sets of labeled arguments in a function call;
* geometric shapes (each field is a shape attribute);
* general application data structures;
* libraries or modules, where the fields represent APIs, not data;
* module namespaces, where the fields are modules.

Record Constructors
~~~~~~~~~~~~~~~~~~~
A record constructor is an expression that constructs a record value.
There are three syntaxes:

1. Dynamic records like ``{a:1, b:2}``.
2. Scoped records like ``{a=1, b=2}``.
3. Directory records. For example, a directory ``foo`` contains two files
   named ``a.curv`` and ``b.curv``. The expression ``file "foo"``
   reads this directory and returns a record value containing
   fields ``a`` and ``b``.

**Dynamic records** permit you to construct a record value dynamically.
The set of field names is determined at run time using program logic, which
can include if statements, local variables and loops. Using the spread
operator (``...``) you can copy the fields from one record into another,
defining default values for missing fields and overriding existing fields.

The simplest case is a list of field expressions (``name:value``)
separated by commas and surrounded by braces.
The full syntax is defined in `Generators`_.

Unlike scoped records, dynamic records do not introduce a scope
(field specifiers can't refer to one another).
This means you can write a dynamic record that references local variables
with the same names as the field names, like this: ``{x:x, y:y}``.
This makes dynamic records the preferred syntax for specifying labeled
arguments in a function call. For example::

    rotate {angle: 45*deg, axis: Z_axis} cube

A **scoped record** is a set of mutually recursive definitions,
separated by commas or semicolons, and enclosed in braces.
See `Definitions`_ for more details.

The set of field names in a scoped record is determined at compile time.
The definitions can refer to one another, but their order doesn't matter,
and you can define recursive functions. Scoped record syntax is used to define
modules, where the fields represent APIs, not data.
See ``lib/curv/*.curv`` in the source tree for examples.
Note that these source files begin and end with ``{`` and ``}``.

**Directory syntax** is used to structure Curv programs consisting of
multiple source files. See `File_Import`_ for details.

.. _`Generators`: Generators.rst
.. _`Definitions`: Blocks.rst
.. _`File_Import`: File_Import.rst

Record Operations
~~~~~~~~~~~~~~~~~
``record . identifier``
  The value of the field named by ``identifier``.
  Eg, ``r.foo``.

``record .[ symbolExpr ]``
  The value of the field named by the symbol after evaluating symbolExpr.
  This allows the field name to be computed at run time.
  Eg, ``r.[#foo]``.

``defined (record . identifier)``
  True if a field named ``identifier`` is defined by ``record``, otherwise false.

``defined (record .[ symbolExpr ])``
  Test the field named by the symbol after evaluating symbolExpr.
  If the field exists, return true, otherwise false.
  This allows the field name to be computed at run time.

``fields record``
  The field names defined by ``record`` (as a list of symbols).

``is_record value``
  True if the value is a record, false otherwise.

``merge listOfRecords``
  Merge all of the fields defined by the records in ``listOfRecords``
  into a single record. If the same field is defined more than once,
  the last occurrence of the field wins.
  Same as::

    {for (r in listOfRecords) ...r}
