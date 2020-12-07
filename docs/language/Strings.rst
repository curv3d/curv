Characters and Strings
----------------------
A string is a list of characters.
The string literal syntax ``"abc"``
is a shorthand for ``[char 97, char 98, char 99]``.
Because there is no difference between a string and a list of characters,
all of the `List`_ operations can be used on strings, without restriction.

Currently, only ASCII characters are supported (excluding the NUL character).
Later, Unicode will be supported.

.. _`List`: List.rst

``char c``
  Construct a character value.
  
  * The argument ``c`` can be an integer code point between 1 and 127.
    For example, ``char 65`` is the character ``A``.
  * The argument ``c`` can be a single character string.
    For example, ``char "A"`` is also the character ``A``.
  * If ``char`` is passed a list of valid arguments, it will return a list
    of results. So ``char`` will convert a list of code points to a string.
    For example, ``char[97,98,99]`` is the string ``"abc"``.

``ucode c``
  Convert a character to a Unicode code point.
  For example, ``ucode(char "A")`` is ``65``.

  If ``ucode`` is passed a list of valid arguments, it will return a list
  of results.
  So ``ucode`` will convert a string to a list of code points, the inverse
  of the ``char`` function.
  For example, ``ucode "abc"`` is ``[97,98,99]``.

``is_string value``
  True if the value is a list of zero or more characters, false otherwise.
  Because there is no difference between a string and a list of characters,
  ``is_string []`` is true.

``repr x``
  Convert an arbitrary value ``x`` to a string.
  This is done by constructing an expression that, when evaluated, reproduces
  the original value. The exception to this is function values:
  currently ``repr`` converts functions to the string ``"<function>"``.

  The ``repr`` function is used to print values in the interactive command
  line interface. It is used to convert values to strings in ``$(expr)``
  substitutions in string literals.

``string x``
  Convert an arbitrary value ``x`` to a string.
  A string is represented by itself.
  A symbol is represented by the characters in its name, without the ``#``
  prefix of symbol literals. Other values are converted as if by ``repr``.

  The ``string`` function is used to convert values to strings
  in ``${expr}`` substitutions in string literals.

``strcat list``
  Each element of the list argument is converted to a string using the
  ``string`` function. Then all of the resulting strings are catenated
  to create the result.

  The ``strcat`` function is used to explain the meaning of ``${...}``
  substitutions within string literals, in the general case.

Simple String Literals
  A simple string literal is a sequence of zero or more literal characters
  enclosed in double quotes.
  For example, ``"Hello, world!"`` is a string literal.

  To include a ``"`` character, use the escape sequence ``"_``.

  To include a ``$`` character, use the escape sequence ``$_``.
  A ``$`` character only needs to be escaped if it is followed by
  ``_``, ``(``, ``[``, ``{`` or an alphabetic character.

  See also: `Rationale for the character escape syntax`_.

.. _`Rationale for the character escape syntax`: rationale/Char_Escape.rst

String Literals with Variable Substitutions
  Curv supports variable substitutions beginning with ``$``.
  Within a string literal, ``$name`` or ``${name}`` substitutes the value
  of the variable ``name`` into the string.
  
  For example, you can debug a Curv program using print statements that
  print the values of variables::
  
    print "x=$x, y=$y"

Character Names
  These are predefined names for character values,
  suitable for substituting into a string literal.
  
  ``nl``
    newline character
  ``tab``
    tab character
  ``quot``
    double quote character (``"``)
  ``dol``
    dollar sign character (``$``)

  For example, instead of using the character escapes ``"_`` and ``$_``,
  you can use ``${quot}`` and ``${dol}`` to insert
  a literal ``"`` or ``$`` into a string literal.

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

Expression Substitutions and String Comprehensions
  These escape sequences evaluate arbitrary expressions,
  and substitute the resulting values into string literals.
  
  ``${...}``
    If ``...`` is an expression, then this escape sequence is
    replaced by the string ``string(...)``. For example,
    ``${2+2}`` is replaced by the character ``4``.

    In the general case, ``...`` is a sequence of zero or more expressions
    or statements, separated by commas or semicolons.
    Inside the braces, you can include comments, newlines, and quoted
    string literals. This feature is called "string comprehensions".
    Then ``${...}`` is replaced by the string ``strcat[...]``,
    where ``[...]`` is a list comprehension. For example,

    * ``${if (cond) "foo"}`` executes the ``if`` statement, interpolating
      ``"foo"`` into the string if ``cond`` is true.
    
  ``$(...)``
    Replaced by the string ``repr(...)``.
    
    ``$(expr)`` is similar to ``${expr}``, except that if ``expr`` evaluates to
    a string, then a quoted string literal will be interpolated.
    This is useful for interpolating the value of a variable in a debug print
    statement.

  ``$[...]``
    Replaced by the string ``char[...]``. For example,
    ``$[65]`` or ``$[0x41]`` is replaced by the character ``A``,
    since ``65`` is the ASCII encoding of ``A``.
    
    More generally, the ``...`` is a list comprehension,
    so ``$[65,66,67,68,69]`` or ``$[... 65..69]``
    are replaced by the characters ``ABCDE``.

String Literal Grammar
  A string literal is enclosed in double-quotes (``"`` characters)
  and contains a sequence of zero or more segments:
  
  * An ASCII character that is not ``"``, ``$``, NUL or newline
    is treated literally,
    and added to the string under construction with no interpretation.
  * ``"_`` is replaced by a ``"`` character.
  * ``$_`` is replaced by a ``$`` character.
  * ``${...}`` is replaced by the string ``strcat[...]``.
  * ``$(...)`` is replaced by the string ``repr(...)``.
  * ``$[...]`` is replaced by the string ``ucode[...]``.
  * ``$identifier`` is equivalent to ``${identifier}``.
  * A ``$`` character that is not followed by ``_``, ``{``, ``(``, ``[``
    or an alphabetic character is treated literally.
  * The sequence <*newline*, *optional-spaces-and-tabs*, ``|``>
    is replaced by a newline.
  * The sequence <*newline*, *optional-spaces-and-tabs*, ``"``>
    is replaced by a newline, and the ``"`` character denotes the end
    of the string literal.

