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
can include conditionals, local variables and loops.

A dynamic record constructor is a list of field generators
(such as ``name: value``), separated by commas and enclosed by braces.
For example::

    {}
    {a: 1}
    {a: 1, b: 2}

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
and you can define recursive functions. Duplicate definitions cause
an error.

Scoped record syntax is used to define
modules, where the fields represent APIs, not data.
See ``lib/curv/*.curv`` in the source tree for examples.
Note that these source files begin and end with ``{`` and ``}``.

**Directory syntax** is used to define a module namespace,
where each module in the namespace is represented by a separate
source file or directory.
See `File_Import`_ for details.

.. _`Generators`: Generators.rst
.. _`Definitions`: Definitions.rst
.. _`File_Import`: File_Import.rst

The Spread Operator
~~~~~~~~~~~~~~~~~~~
Within a dynamic record constructor, fields are processed left to right.
If the same field name is specified more than once, then the last occurrence
wins. For example::

    {a: 1, b: 2, c: 3, a: 999}

evaluates to ``{a: 999, b: 2, c: 3}``.

The **spread operator** ``...r`` copies the fields from the record ``r``
into the record being constructed. Using the left-to-right override
semantics, you can create an updated copy of an existing record ``r``,
defining default values for missing fields, and overriding existing fields.

* ``{x: 0, ... r}`` -- The same as record ``r``, except that if field ``x`` is
  not defined, then it defaults to ``0``.
* ``{... r, x: 0}`` -- The same as record ``r``, extended with field ``x``,
  with ``x:0`` overriding any previous binding.

Within a list constructor, ``...r`` copies the fields of the record ``r``
into the list, converting each field into a ``[symbol,value]`` pair.
So ``[...r]`` converts a record to a field list. For example::

    [... {a: 1, b: 2}]

evaluates to ``[[#a,1], [#b,2]]``.

Within a dynamic record constructor, ``...fieldlist`` copies the fields
from a fieldlist into the record being constructed.
So ``{...fieldlist}`` converts a fieldlist to a record.

The spread operator is one example of generator syntax.
See `Generators`_ for full details.

Record Operations
~~~~~~~~~~~~~~~~~
In the following examples, ``R`` is a local variable
containing the record value ``{a: 1, b: 2}``.

``is_record value``
  True if the value is a record, false otherwise.
  For example, ``is_record R`` is ``#true``.

``record . identifier``
  The value of the field named by ``identifier``.
  For example, ``R.a`` is ``1``.

``record .[ symbolExpr ]``
  The value of the field named by the symbol after evaluating symbolExpr.
  This allows the field name to be computed at run time.
  For example, ``R.[#a]`` is ``1``.
  See `Trees`_ for more information about querying data structures.

``recordVariable . identifier := newValue``
  Given a local variable containing a record value,
  update the variable to contain a copy of the record value
  where the field named ``identifier`` has the value ``newValue``.
  For example, after executing::
  
     R.a := 99
  
  then the variable ``R`` will contain ``{a: 99, b: 2}``.

``recordVariable .[ symbolExpr ] := newValue``
  Given a local variable containing a record value,
  update the variable to contain a copy of the record value
  where the field named by the symbol ``symbolExpr`` has the value ``newValue``.
  For example, after executing::
  
     R.[#a] := 99
  
  then the variable ``R`` will contain ``{a: 99, b: 2}``.
  See `Trees`_ for more information about updating data structures.

``defined (record . identifier)``
  True if a field named ``identifier`` is defined by ``record``, otherwise false.
  For example, ``defined(R.a)`` is true and ``defined(R.foo)`` is false.

``defined (record .[ symbolExpr ])``
  Test the field named by the symbol after evaluating symbolExpr.
  If the field exists, return true, otherwise false.
  This allows the field name to be computed at run time.
  For example, ``defined(R.[#a])`` is true and ``defined(R.[#foo])`` is false.

``fields record``
  The field names defined by ``record`` (as a list of symbols).
  For example, ``fields R`` returns ``[#a, #b]``.

``merge listOfRecords``
  Merge all of the fields defined by the records in ``listOfRecords``
  into a single record. If the same field is defined more than once,
  the last occurrence of the field wins.
  Same as::

    {for (r in listOfRecords) ...r}

.. _`Trees`: Trees.rst
