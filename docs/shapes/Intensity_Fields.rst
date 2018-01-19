Intensity Fields
================
An intensity field is a function that maps each point in space and time [x,y,z,t]
onto an intensity (a number between 0 and 1).
The ``t`` parameter allows intensity fields to be animated.

An intensity field can be composed with a colour map
to construct a colour field. Which is a colour pattern that can be
applied to a shape.

*Fractal noise* is available as a family of intensity fields.
This can be used to create geometry, and not just colour fields.

Future Work
-----------
* Constructors, that build intensity fields.
* Transformations, that map one ifield onto another.
* Combinators, that combine two or more ifields, creating a new ifield.
