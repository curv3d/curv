# String Values

## Unicode
Strings are represented internally as UTF-8.
There are no string operations that can create an invalid UTF-8 sequence
in a string.

In OpenSCAD, len(str) returns the number of code points,
and str[i] returns the i'th code point.
These have O(n) complexity. That is a "good enough" design.
It lets you introspect a string without creating corrupt UTF-8.

Perl6 counts grapheme clusters, not code points.
This seems better, especially in the context of the text() primitive.

The utf8everywhere manifesto says that random access to character strings
is not very important for text processing, and that concerns about
character counting are overblown. There's not a single right way to do it,
and for some use cases, you need to count code units. Also, for UTF-8,
using a data structure that efficiently addresses individual code points
isn't important for most text processing. Eg, you can search for a substring

The requirements of Curv are likely to be pretty specific, though?
I don't see a general need for text processing in Curv.
I can imagine people using it for very application specific (and thus
unpredictable) purposes, like maybe parsing strings in imported JSON data.

## List of Char
Curv would be "more elegant" (for some values of elegant) if strings
were lists of "characters" (aka code points). So "abc" == ['a','b','c']
and "abc"@0 == 'a' and ""==[] and concat works on strings and for iterates
over the characters in a string. A character (code point) is new data type,
represented as an immediate Value.

This would give us powerful string processing using generic list operations.

It would also permit mixed lists of characters and non-characters.
I'd need an unambiguous way of printing such lists.
This adds extra complexity (and confusion, no other language works this way).

Defining a character literal to be a code point is potentially broken,
given that visually, grapheme clusters are characters, and are the unit
of text selection and cursor movement.
* You get an error if you try to put a grapheme cluster into a char literal
* What does a char literal for a combining character even look like?

It makes little sense to reverse a list of code points. Combining characters
are messed up. It makes more sense to reverse a list of grapheme clusters.

## Multiple Viewpoints
Maybe, a string is not a list. But, there are multiple ways of viewing
a string as a list of items: code points, grapheme clusters, and larger units.
There are operations for splitting a string into a list of substrings, then you
apply list operations, then you strcat the list back to a string.
