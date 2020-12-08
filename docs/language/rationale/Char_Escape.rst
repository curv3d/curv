Special Character Escape Sequences
==================================

Using ``_`` to escape special characters
----------------------------------------
Within a string literal, the special characters ``$`` and ``"``
are made literal by suffixing them with an ``_`` character.
So you write ``$_`` or ``"_`` to get a literal ``$`` or ``"``.

For example, the following text::

    print "x=$x";

can be placed in a string literal by writing::

    "print "_x=$_x"_;"

Similarly, you use ``'_`` to include a literal ``'`` in a quoted
identifier. For example::

    'This isn'_t C'

Rationale for this syntax
-------------------------
The de facto industry standard is to use the C convention: a preceding ``\``
character escapes special characters. Curv's trailing ``_`` convention has
technical advantages over the C convention.

1. The ``_`` character never needs to be escaped.
   At present, ``'_`` is the only escape sequence that is provided or needed
   within a quoted identifier. If I instead used the backslash convention,
   then now there are two special characters that need to be escaped.

2. It doesn't interfere with other syntaxes.
   By contrast, because ``\`` is a prefix escape character, it interferes
   with syntaxes that use ``\``. For example, you would need to type ``\\``
   for the path separator in Windows filenames.

3. The new syntax has less visual clutter.
   In order for escaped text to be readable, the literal
   characters need to be visually prominent, and the escape characters
   need to recede into the background.
   The ``_`` character was chosen so that it doesn't interfere visually
   with the characters it is escaping.

4. The syntax is clearer when text is repeatedly escaped.
   Using C syntax, escape sequences grow exponentially in size as they
   are repeatedly escaped::

       \  ->  \\  ->  \\\\  ->  \\\\\\\\
       "  ->  \"  ->  \\\"  ->  \\\\\\\"

   Using the ``_`` syntax, escape sequences grow linearly in size as they
   are repeatedly escaped::

       $  ->  $_  ->  $__  ->  $___
       "  ->  "_  ->  "__  ->  "___
