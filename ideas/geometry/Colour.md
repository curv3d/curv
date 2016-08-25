# Colour

1. I'd like to support multiple colours in the manner
   of the "parts, materials and colours" proposal for OpenSCAD.
   This means: the shape is partitioned into disjoint zones,
   each with a different solid colour.
   Each CSG operation is extended to define how the colour zones of the
   output are determined based on the colour zones of the input.
2. I'd like to support "full colour printing" for printers like the ORD
   colour mixing printer, or material jet printers. This means pairing a
   distance field with a colour field (a function that maps each [x,y,z]
   to a colour). Once again, CSG operations need to define how the colour field
   of the output is computed from the colour fields of the inputs.

\#2 implies the ability to compare functions for equality. Or, that would
be handy for merging adjacent zones with the same colour field, for
efficiency reasons? So all I need is a conservative equality function
for which f==g is true only if f and g are guaranteed to be the same,
while false means there is room for doubt. I'm not sure how to define
this equality function while preserving declarative semantics/referential
transparency:
 * "extensional equality" is not computable. Means that equal functions
   are operationally equivalent, they map a given input to the same output.
 * "intensional equality" compares the structure of two functions.
   It's not declarative, since otherwise legal program transformations
   break equality by altering the program structure that function equality
   compares. For example, x+y and x==y may no longer be commutative if
   you implement intensional function equality.
 * "pointer equality" is cheap, and not referentially transparent.
   It would be the best choice if function equality is being used
   to boost the performance of CSG algorithms.
   I can't use these semantics for `==` because it would violate the
   algebraic properties, but I could add a kludge like fun_eq_kludge(f1,f2).
