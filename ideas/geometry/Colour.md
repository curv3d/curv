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
* 2D shape with colour[x,y] function.
  * Either a shape has colour or it doesn't. Union requires either both args
    have colour, or neither.
  * When a coloured shape is transformed, the transformation is also applied
    to the colour field.
* `colour(c) shape` applies a colour or a colour field to a shape.
* Colour operations:
  * predefined colour values (red,orange,green,...)
  * colour operations (eg, light red)
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

## Implementation (Coloured Union)
Currently, the colour shape attribute is optional.
```
Union2(s1,s2) = (
    let(d p = min[s1.dist p, s2.dist p],
        b = [min[s1.bbox'0, s2.bbox'0], max[s1.bbox'1, s2.bbox'1]],
        c = if (!defined(s1.colour) && !defined(s2.colour))
                null
            else
                let(c1 = if (defined(s1.colour)) s1.colour else p->black,
                    c2 = if (defined(s2.colour)) s2.colour else p->black)
                p->if(s1.dist p <= 0) c1 p else c2 p)
    if (c == null)
        shape2d {dist=d, bbox=b}
    else
        shape2d {dist=d, bbox=b, colour=c}
);
Union = reduce(nothing, Union2);
```
Therefore, Union needs to conditionally add a 'colour' field to the shape it
constructs. How?
   1. if (defined(s1.colour) && defined(s2.colour))
        shape2d{dist=d,bbox=b,colour=c}
      else
        shape2d{dist=d,bbox=b}
   2. {dist=d,bbox=b,if(c!=null)colour=c}, by analogy with lists.
      Makes sense in the context of adding pairs to a dictionary, less sense
      for definitions within a scope.
      {dist:d,bbox:b,if(c!=null)colour:c}
      `id:expr` is a special Meaning that adds a binding pair to a dictionary
      under construction. This happens at runtime.
   3. If shape.colour==null then it's ignored.
      a. That means more code for union.
         is_coloured(shape)=defined(shape.colour)&&shape.colour!=null;
      b. Or the `colour` attribute is always present, but defaults to null
         if not specified.
4. Or, the colour field is always present, and defaults to `p->black`.

Suppose we use the simplest approach, 4. Then union becomes much simpler.
```
_union2(s1,s2) = shape2d {
    dist p = min[s1.dist p, s2.dist p],
    colour p = if (s1.dist p <= 0) s1.colour p else s2.colour p,
    bbox = [min[s1.bbox'0, s2.bbox'0], max[s1.bbox'1, s2.bbox'1]],
};
```
Hard to make it simpler than that.

What about optimizing out the `if` if both colours are the same?
Easy, do it in the GL compiler, not at a higher level. If both branches
of the `if` are the same SSA variable, then replace the `if` with that
SSA variable. For now, assume the GPU driver does this. Later, do it in
a GL optimizer.
