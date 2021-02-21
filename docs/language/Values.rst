Values
------
In Curv, everything is a value.
All values are immutable (there are no mutable objects).
All data structures are hierarchical and finite, and therefore printable.
This design make Curv programs easier to understand and debug.

Curv has 6 primitive value types:

==============     ============================================
symbol             ``#foo``, ``#'hello world'``
number             ``-1, 3.1416, 0xFF``
character          ``char 65`` or ``#"A"``
list               ``[0,1,2]`` or ``"abc"``
record             ``{name: "Lancelot", quest: "To seek the holy grail", favourite_colour: "blue"}``
function           ``x -> x+1``
==============     ============================================

There are no type declarations or class declarations.
All application data is represented using the 6 value types.
For example,

* The boolean values are represented by the symbols ``#true`` and ``#false``.
* If you need a null value, like the ``null`` in JSON or Javascript,
  you can use the symbol ``#null``.
* A character string is a list of characters.
* A geometric point or vector is a list of numbers.
* A matrix is a list of lists of numbers.
* A geometric shape is a record containing 5 fields
  (two booleans, two functions and a matrix).
