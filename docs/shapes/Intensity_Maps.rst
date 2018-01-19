Intensity Maps
==============
An intensity map is a function that maps [x,y,z,t]
onto an intensity (a number between 0 and 1).
The ``t`` parameter is time: intensity maps may be animated.

An intensity map can be composed with a colour map
to construct a colour field. Which is a colour pattern that can be
applied to a shape.

*Fractal noise* is available as a family of intensity maps.
This can be used to create geometry, and not just colour fields.

Future Work
-----------
* Constructors, that build intensity maps.
* Transformations, that map one imap onto another.
* Combinators, that combine two or more imaps, creating a new imap.
