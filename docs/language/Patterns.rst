Patterns
========

introduction and example

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
  A record pattern.

``_``
  Wildcard pattern. Matches any value, but the value is ignored, and no names are bound.

*predicate* *pattern*
  Predicate pattern.
  First, evaluate the *predicate* expression, which must yield a function that returns ``true`` or ``false``.
  Call the predicate function with the to-be-matched value as an argument.
  If the predicate is false, the match fails. If the predicate is true,
  then match the value against *pattern*.

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
