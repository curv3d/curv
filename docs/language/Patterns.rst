Patterns
========

Patterns are a concise syntax for testing that a value has a certain structure,
and then extracting information from values that match the pattern.

In a data definition like::

   x = 42;

``x`` is a pattern. It's a very simple pattern, that matches any value,
and binds that value to the variable ``x``.

We can constrain the type of value being bound using a predicate pattern::

   (is_num x) = some_complex_expression;

This definition asserts that ``some_complex_expression`` returns a number.
If the assertion fails, the pattern match fails and an error is reported.
The predicate ``is_num``
can be replaced by any function that returns true or false.

We can use patterns to decompose lists or records into their component values.
For example::

   (x,y,z) = some_expression_returning_a_3_list;

In a function definition like::

    f x = x + 1;

the parameter ``x`` is a pattern. We can replace this with a predicate pattern::

    f (is_num x) = x + 1;

Function definitions can use list or record patterns.
Here is a function whose first argument is a list.
The list is decomposed into 3 elements using a list pattern::

    f (shape, angle, point) = ...;

List and record patterns can be nested, so you can also write::

    f (shape, angle, [x,y,z]) = ...;

If a function argument consists of a list of unrelated values of different
types, it's better to use named arguments than positional arguments.
It's recommended to use a record pattern instead::

    f {shape, angle, point: [x,y,z]} = ...;

Pattern Syntax
--------------

*identifier*
  Match any value, bind the value to *identifier*.

``_``
  Wildcard pattern. Match any value, but the value is ignored,
  and no names are bound.

*predicate* *pattern*
  Predicate pattern.
  First, evaluate the *predicate* expression, which must yield a function
  that returns ``true`` or ``false``.
  Call the predicate function with the to-be-matched value as an argument.
  If the predicate is false, the match fails. If the predicate is true,
  then match the value against *pattern*.

``[`` *pattern1* ``,`` ... ``]``
  List pattern with *n* elements, each of which is a pattern.
  Matches a list value with exactly *n* elements.
  The first element of the list value is matched against *pattern1*, and so on.
  The element patterns are separated or terminated by commas or semicolons.

``(`` *pattern1* ``,`` ... ``)``
  A list pattern, equivalent to the ``[`` ... ``]`` case.
  A parenthesized list pattern is either empty ``()``
  or it contains at least one comma or semicolon.

``{`` *field_pattern1* ``,`` ... ``}``
  A record pattern contains a list of field patterns::
  
    field_pattern ::=
        fieldname ':' |
        fieldname ':' pattern |
        fieldname ':' pattern '=' default_value |
        identifier_pattern |
        identifier_pattern '=' default_value
    fieldname ::= identifier | quoted_string
    identifier_pattern ::=
        identifier |
        predicate identifier_pattern |
        '(' identifier_pattern ')'

  Each field pattern matches one field in a record value.
  The field patterns are separated or terminated by commas or semicolons.

  In the general case, a field pattern separately specifies a field name
  and a pattern to match against the field value.
  The field name can be either an identifier or a string literal.
  For example, ``{rotate_by: angle, around: [x,y,z]}``
  is a record pattern that requires two fields, ``rotate_by`` and ``around``.
  The value of the first field is bound to the variable ``angle``.
  The value of the second field is a point that is decomposed into a 3 element
  vector, binding the elements to the variables ``x``, ``y`` and ``z``.

  As a special case, the field name and the pattern can be combined into
  a single *identifier pattern*, which contains a single identifier that
  is both the field name, and the variable to which the value is bound.
  An identifier pattern is just an identifier, optionally preceded by a
  predicate. For example, ``{a, is_num n}`` matches a record containing
  the fields ``a`` and ``n``, and binds the field values to variables of
  the same names. The value of ``n`` is required to be a number.

  If a field pattern is suffixed with ``= expr``, then if the record being
  matched doesn't contain a field of the expected name, then ``expr`` is used
  as the default value of the field.

  ``name:`` is a special field pattern that matches a field ``name:true``,
  and binds no variables.
  
  If the record value contains fields that are not matched by any field pattern,
  then the pattern match fails.

``(`` *pattern* ``)``
  Patterns can be parenthesized, without changing their meaning.

Pattern Use Cases
-----------------

*pattern* ``->`` *expression*
  Function literal.

*pattern* ``=`` *expression*
  Definition.

``for (`` *pattern* ``in`` *list_expression* ``)`` *statement*
  ``for`` statement.

*pattern* ``:`` *expression*
  Field specifier in a record literal.

``match [`` *pattern1* ``->`` *expression1* ``,`` ... ``]``
  A multi-branch conditional that uses pattern matching.
