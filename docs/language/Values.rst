Values
------
In Curv, everything is a value.
All values are immutable (there are no mutable objects).
All data structures are hierarchical and finite, and therefore printable.
This design make Curv programs easier to understand and debug.

Curv has 7 primitive value types:

==============     ============================================
null               ``null``                
boolean            ``true, false``
number             ``-1, 3.1416, 0xFF``
string             ``"hello, world"``
list               ``[0,1,2]``
record             ``{name: "Lancelot", quest: "To seek the holy grail", favourite_colour: "blue"}``
function           ``x -> x+1``
==============     ============================================

The first 6 data types are taken from JSON,
and Curv can be considered a superset of JSON.

There are no type declarations or class declarations.
All application data is represented using the 7 value types.
For example, a geometric point or vector is a list of numbers.
A matrix is a list of lists of numbers. A geometric shape is a record
containing 5 fields (two booleans, two functions and a matrix).
