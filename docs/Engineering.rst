Engineering
===========

Curv offers two libraries to facilitate engineering: sketch and builder.

Builder is for combining 3D solids or 2D faces in a relative coordinate system.
It makes cutting or intersecting shapes easier by setting combiner modes, such
as union, difference and intersection. Operations like distributing cylinders
along a circle and cutting them out should be done using this library.

Sketch is for combining edges to create complex 2D faces. Each edge function
may take relative or absolute coordinates, so pay attention! It was designed
after the interface offered by CADQuery, since it has a significant enough
following. This library is particularly useful for defining gears, or ports,
or other measurement specific faces.

See the cheatsheet.txt for a quick reference.

Here is a short usage example of using both libraries at once:

```
let include lib.builder; include lib.sketch; in
build
>> put (box) [0,0,0]
>> put (sphere) [2,0,0]
>> child (ctx -> ctx
  >> put (
    workplane
    >> move_abs [0, 3]
    >> spline [[[1, 3.5], [2, 2.6], [3,3]]]
    >> tangent_arc_point [6,3]
    >> tangent_arc_point [8,1]
    >> spline [[[7,-3], [6,0], [3, 0]]]
    >> close
    >> extrude 1
  ) [0,-2,0] // Remember, origin is [2,0,0] since it's child of sphere!
)
>> done
```
