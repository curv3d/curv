Strings
-------
A string is a sequence of characters.
For example, ``"Hello, world"`` is a string literal.

Currently, only ASCII characters are supported (excluding the NUL character).
Later, Unicode will be supported.

``str[i]``
  String indexing: yields a string consisting of the i'th character of ``str``, with 0-based indexing.
  Individual characters are represented by strings, there is no separate 'character' data type.

``str[indices]``
  String slicing. ``indices`` is a list of character indexes.
  A new string is returned containing each indexed character from ``str``.
  For example, ``"foobar"[0..<3]`` yields ``"foo"``.

``count str``
  The number of characters in the string ``str``.

``is_string value``
  True if the value is a string, false otherwise.

``repr x``
  Convert an arbitrary value ``x`` to a string.
  This is done by constructing an expression that, when evaluated, reproduces the
  original value. The exception to this is function values: currently ``repr``
  converts functions to the string ``"<function>"``.

``strcat list``
  String concatenation. ``list`` is a list of arbitrary values.
  Each element of ``list`` that isn't already a string is converted to a string using ``repr``.
  Then the resulting list of strings is concatenated into a single string.

``encode str``
  Convert a string to a list of Unicode code points (represented as integers).

``decode codelist``
  Convert a list of Unicode code points to a string.
  Currently the only supported code points are 1 to 127.

``nl``
  A string containing a single newline character.
  It is intended for substitution into a string literal.

String Constructor
  A string constructor is enclosed in double-quotes (``"`` characters)
  and contains a sequence of zero or more segments.
  
  * A literal segment is a sequence of ASCII characters,
    excluding the characters NUL, ``"`` and ``$``,
    which are added to the string under construction with no interpretation.
  * ``""`` is interpreted as a single ``"`` character.
  * ``$$`` is interpreted as a single ``$`` character.
  * ``${expr}`` interpolates the value of ``expr``. If the value is not a string,
    then it is converted to a string by ``repr``.
  * ``$(expr)`` is equivalent to ``${repr(expr)}``.
  * ``$[...]`` is equivalent to ``${decode[...]}``.
  * ``$identifier`` is equivalent to ``${identifier}``.
