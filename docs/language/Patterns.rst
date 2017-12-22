Patterns
========

Patterns are a concise syntax for testing that a value has a certain structure,
and then extracting information from values that match the pattern.

example

Pattern Syntax
--------------

*identifier*
  Match all values, bind the value to *identifier*.

``[`` *pattern1* ``,`` ... ``]``
  List pattern with *n* elements, each of which is a pattern.
  Matches a list value with exactly *n* elements.
  The first element of the list value is matched against *pattern1*,
  and so on.

``(`` *pattern1* ``,`` ... ``)``
  A list pattern, equivalent to the ``[`` ... ``]`` case.
  A parenthesized list pattern is either empty ``()``
  or it contains at least one comma.

``{`` *field_pattern1* ``,`` ... ``}``
  A record pattern contains a list of field patterns::
  
    field_pattern ::= field | field '=' default_value | field_name ':'
    field ::= identifier | field_name ':' pattern
    field_name ::= identifier | quoted_string

  Each field pattern matches one field.
  
  * The simplest field pattern is an identifier. For example, ``foo``
    matches a field named ``foo``, and binds its value to ``foo``.
  * The field name can be different from the variable name that you bind its value to.
    For example, ``{angle: a, axis: v}`` is a record pattern that requires two fields
    named ``angle`` and ``axis``, but the values of those fields are bound to the
    variables ``a`` and ``v``.
  
``_``
  Wildcard pattern. Matches any value, but the value is ignored, and no names are bound.

*predicate* *pattern*
  Predicate pattern.
  First, evaluate the *predicate* expression, which must yield a function that returns ``true`` or ``false``.
  Call the predicate function with the to-be-matched value as an argument.
  If the predicate is false, the match fails. If the predicate is true,
  then match the value against *pattern*.

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
  Field spec.

``match [`` *pattern1* ``->`` *expression1* ``,`` ... ``]``
  A multi-branch conditional that uses pattern matching.
