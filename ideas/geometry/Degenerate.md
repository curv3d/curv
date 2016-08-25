# Degenerate Shapes

A degenerate 3D shape is one that has a zero volume. For example,
* the empty set, containing no points
* a single geometric point (zero dimensional)
* a line segment, which has extension, but no height or depth (1 dimensional)
* more generally, a curved path through 3-space
* a plane segment (2D), or more generally a curved surface

Degenerate shapes can't be 3D printed.
But, like infinite shapes, they can be useful as intermediate values
when computing a 3D printable result.
One example recently discussed on the OpenSCAD forum:
    ```
    minkowski() {
        scale($t) sphere();
        cube();
    }
    ```
At `$t=0`, the sphere degenerates to the point [0,0,0].
If this degeneracy is handled correctly, then there is no discontinuity
in the animation, and you get `cube()` at `$t=0`.
And this example happens to work in OpenSCAD, although the behaviour
around degenerate shapes in general is flakey.

F-Rep can represent any degenerate shape, and many F-Rep operations will
naturally work correctly in their presence. But, it's also going to be
a source of buggy and unexpected behaviour.

Pragmatically, it makes sense to treat all degenerate shapes the same way,
as things that don't exist and that should be suppressed.
* Some F-Rep operations may leave behind "ghosts of departed shapes" in the
  distance field. These are local minima that, while positive and representing
  empty space, could be inflated into shapes with non-zero volume.
  If this happens, it's a bug and an artifact of a wonky distance field.
  We need code to clean up these anomalies and prevent rendering bugs.
* ImplicitCAD appears to do this.
  It appears to represent all degenerate shapes
  with the bounding box `[[0,0,0],[0,0,0]]`.
  It has code that detects cases where a degenerate shape
  would be generated, and maps those to the generic degenerate shape.
  Other code tests for this bbox and handles it appropriately;
  eg `union` ignores degenerate shapes.
* OpenSCAD represents all shapes as polyhedral meshes.
  A well formed mesh can't have zero-area triangles,
  so it makes sense to treat all degenerate shapes as the absence of geometry.
  The NEF polyhedron representation does support 1 degenerate case, AFAIK,
  but not more than one.
* In practice, OpenSCAD does support a family of different degenerate shapes.
  I can create single-point degenerate shapes using
  `transform(pt) scale(0) cube()`, and these work correctly with minkowski().
  However, this is buggy and can produce CGAL assertions (I guess this
  representation doesn't survive conversion to a Nef Polyhedron).
* In theory, F-Rep systems can represent the entire infinite family
  of degenerate shapes. In practice, ray casting can't find degenerate shapes,
  and it makes sense that during rendering, any point, tendril or sheet thinner
  than the rendering threshold will disappear.

Mathematically, there are geometric operations that care about the structure
of degenerate shapes. A pure, mathematically correct geometry kernel would
properly represent the structure of degenerate shapes, and geometric operations
would handle these cases correctly.

`nothing` is a degenerate shape containing no geometric points: the empty set.
```
nothing = shape3d {
  dist(req)(p) = inf,
  bbox = [[inf,inf,inf], [-inf,-inf,-inf]]
};
```

`origin` is a degenerate shape containing the single geometric point [0,0,0]
and nothing else.
```
origin = cube(0);
origin = sphere(0);
origin = shape3d {
  dist(req)[x,y,z] = sqrt(x^2 + y^2 + z^2),
  bbox = [[0,0,0],[0,0,0]]
};
```

For Minkowski Sum, the identity element is `origin`
and the zero element is `nothing`.

The inflate operation is similar.
If you inflate `origin`, you should get a sphere.
But inflating `nothing` should produce `nothing`.

Can the Curv geometry engine represent and distinguish these two shapes?
* If I know, from construction, that a shape is degenerate, then I can
  mark it and treat it specially.
* If a degenerate shape is the result of an operation on polyhedra,
  then it's probably possible to compute and represent a degenerate shape.
  What are the rules for this, though? If I intersect two cubes that share
  a face, then is that face (a 2D object) the intersection?
  If I scale a cube to zero size, then that should be a point.
* If a degenerate shape is the result of an F-Rep operation, then it
  might not be feasible to reconstruct the shape boundary. Ray casting
  isn't guaranteed to work. So, these cases would behave differently from
  "by construction" degenerate shapes.
  * In F-Rep, intersection is max, and the intersection of two cubes sharing
    a face would create a distance field ==0 at the shared face.
    The bbox in this case would be minimal and correct if the cubes are
    axis aligned. An `inflate` of this degenerate shape would have the
    correct behaviour.
  * ImplicitCAD would detect a zero-volume bbox and convert it to a void
    volume, so a subsequent inflate would have no effect. But that would
    only work for an axis aligned square.

In summary, I don't know if any consistent implementation of degenerate
shapes is possible in F-Rep. The full range of degenerate shapes is
definitely representable, and many operations will naturally produce or
consume degenerate shapes with correct behaviour (depending on your
expectations, of course). But some operations won't deal with degenerate
shapes correctly.

All I can do is implement stuff, document behaviour around degenerate shapes,
fix bugs where I find them.
