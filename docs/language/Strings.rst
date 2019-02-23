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

Simple String Literals
  A simple string literal is a sequence of zero or more literal characters
  and variable substitutions, enclosed in double quotes.
  For example, ``"Hello, world!"`` is a string literal.
  
  Curv does not support C-style escape sequences beginning with ``\``.
  That might be confusing on Windows, where the ``\`` character is used
  in pathnames. Instead, Curv supports variable substitutions beginning
  with ``$``. Within a string literal, ``$name`` or ``${name}`` substitutes the value
  of the variable ``name`` into the string.
  
  For example, you can debug a Curv program using print statements that
  print the values of variables::
  
    print "x=$x, y=$y"
  
  To insert a literal ``$`` or ``"`` character into a string literal,
  you can use ``${dol}`` or ``${quot}``.

Character Names
  These are predefined names for single-character strings,
  suitable for substituting into a string literal.
  
  ``nl``
    newline character
  ``tab``
    tab character
  ``quot``
    double quote character (``"``)
  ``dol``
    dollar sign character (``$``)

Multi-Line String Literals
  A multi-line string literal represents multi-line text,
  and can be indented without adding the indentation to the string content.

  For example::
  
    "First line.
    |Second line.
    |Final line with trailing newline.
    "
  
  The ``|`` characters separate the indentation from the string content.
  The last line of our string literal is just ``"``,
  which indicates that the string has a trailing newline.
  If you don't want a trailing newline, you can write this::
  
    "First line.
    |Second line.
    |Final line without trailing newline."

Expression Substitutions
  These escape sequences evaluate arbitrary expressions,
  and substitute the resulting values into string literals.
  
  ``${expr}``
    Interpolate the value of ``expr``.
    If the value is not a string, then it is converted to a string by ``repr``.
  ``$(expr)``
    Similar to ``${expr}``, except that if ``expr`` evaluates to a string,
    then a quoted string literal will be interpolated.
    Useful for interpolating the value of a variable in a debug print statement.
    Equivalent to ``${repr(expr)}``.
  ``$[...]``
    Equivalent to ``${decode[...]}``.
    For example, ``$[65,66,67]`` interpolates the characters ``ABC``,
    since ``65`` is the ASCII encoding of ``A``, and so on.

Compact Escape Sequences for ``$`` and ``"``
  ``$.`` is replaced by ``$``, and ``$=`` is replaced by ``"``.
  As a mneumonic, ``=`` is a sideways ``"`` character.
  
  These escapes are useful in certain technical situations,
  such as generating source code.
  They are less readable than ``${dol}`` and ``${quot}``,
  but they are also more compact. They have an advantage over
  the corresponding escape sequences in C (``\\`` and ``\"``), because if
  these escape sequences are repeatedly escaped, then they grow linearly
  instead of exponentially.
  
  Here is the growth sequence if a ``$`` or ``"`` character
  is repeatedly escaped in Curv::
  
    $ -> $. -> $.. -> $...
    " -> $= -> $.= -> $..=
    
  By contrast, here is the growth sequence if a ``\`` or ``"`` character
  is repeatedly escaped in C::
  
    \ -> \\ -> \\\\ -> \\\\\\\\
    " -> \" -> \\\" -> \\\\\\\"
  
String Literal Grammar
  A string literal is enclosed in double-quotes (``"`` characters)
  and contains a sequence of zero or more segments.
  
  * A literal segment is a sequence of ASCII characters,
    excluding the characters NUL, newline, ``"`` and ``$``,
    which are added to the string under construction with no interpretation.
  * ``$.`` is replaced by a ``$`` character.
  * ``$=`` is replaced by a ``"`` character.
  * ``${expr}`` interpolates the value of ``expr``. If the value is not a string,
    then it is converted to a string by ``repr``.
  * ``$(expr)`` is equivalent to ``${repr(expr)}``.
  * ``$[...]`` is equivalent to ``${decode[...]}``.
  * ``$identifier`` is equivalent to ``${identifier}``.
  * The sequence <*newline*, *optional-spaces-and-tabs*, ``|``>
    is replaced by a newline.
  * The sequence <*newline*, *optional-spaces-and-tabs*, ``"``>
    is replaced by a newline, and the ``"`` character denotes the end
    of the string literal.

