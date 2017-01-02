# Colour

The focus of Curv 1.0 is art and mathematics, while the 2.0 release
has more of an engineering focus.

1. In release 1.0, there is a fully general volumetric colour model,
   where each point on the boundary or interior of a shape is assigned
   a colour by a colour function (aka a "colour field").
2. In release 2.0, I would like to support multi-colour, multi-material
   3D printing in the manner of the "parts, materials and colours" proposal
   for OpenSCAD. This means: the shape is partitioned into disjoint zones,
   each with a different solid colour.

Each CSG operation is extended to define how the colour field of the
result is determined based on the colour fields of the inputs.

Colour:
* Initially, [r,g,b] is a colour value. (Later I'll support alpha.)
* [x,y]->[r,g,b] and [x,y,z]->[r,g,b] are 2d and 3d colour fields.
* 2D shape with color[x,y] function.
  * Either a shape has color or it doesn't. Union requires either both args
    have color, or neither.
  * When a coloured shape is transformed, the transformation is also applied
    to the colour field.
* `color(c) shape` applies a colour or a colour field to a shape.
* Colour operations:
  * predefined color values (red,orange,green,...)
  * color operations (eg, light red)
  * Geometric transforms can be applied to colour fields.
* Image import.
* Colour fields can be animated. Colour functions are passed a time coordinate.

Can colour fields be compared for equality? Maybe that would be used to merge
adjacent zones with the same colour field, for efficiency reasons?
So all I need is a conservative equality function for which f==g is true
only if f and g are guaranteed to be the same, while false means there is
room for doubt. I'm not sure how to define this equality function while
preserving declarative semantics/referential transparency:
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
