# Programming

The Functional Geometry system (FG) provides both a low-level and a high-level API.
* The high level API includes familiar operations like sphere(), translate()
  and intersection(),
  plus new operations like shell(), morph() and perimeter_extrude().
* The low level API allows users to directly define new primitive operations
  using functional representation.

The low level API is quite powerful. Most of the OpenSCAD geometry
primitives can be defined in a few hundred lines of OpenSCAD2.
That's very impressive, compared to the amount of code required to implement
these operations on a mesh.
The low level API is
also subtle and tricky. So you can define `sphere()`, `union()`, etc, with 3
lines of code per primitive, but they can be subtle and tricky lines of code.

This document is a tutorial introduction to the low level API,
which is illustrated by showing you how to define a lot of interesting
high level operations.
There's enough detail so that you can really wrap your head around
the idioms of creating geometry using Functional Representation.

## Benefits and Limitations

When compared to the mesh-based programming model of OpenSCAD,
the Functional Geometry API has both benefits and limitations.

* **expressive power** <br>
  A primary benefit of Functional Geometry is that you can define powerful new
  geometric primitives with only a few lines of OpenSCAD2 code.
  Most of OpenSCAD's geometry kernel can be implemented, quite simply,
  at user level. By contrast, programming these same primitives using a mesh
  is notoriously difficult, with lots of edge conditions to deal with,
  and a lot of additional complexity if you use floating point (as opposed
  to the slow and memory-inefficient rational numbers used by OpenSCAD).

* **direct access to the representation of shapes** <br>
  In OpenSCAD, shapes are opaque: you can't access their representation;
  you can't even query the bounding box, let alone access the vertexes and faces.
  This restriction is due to an engineering tradeoff: we want preview to be fast,
  and we don't generate the mesh representation of CSG operations during preview.
  If we were to provide access to the mesh during preview, then preview would become as slow as rendering.
  Functional Geometry has no such tradeoff, and much of the programming power is due
  to the fact that we provide full access to the underlying functional representation.

* **curved surfaces** <br>
  It's extremely easy to work with curved surfaces in Functional Geometry,
  something that is a major weakness of OpenSCAD.
  * A curved object is represented by its mathematical formula,
    which makes curved surfaces and organic forms easy to define with very little code.
  * Internally, a curved object is represented exactly. It's like programming in
    OpenSCAD with `$fn=∞`. Curved objects can be exactly rendered in the preview window at full resolution,
    even for complex models. You can zoom in many orders of magnitude without the abstraction
    breaking down, limited only by floating point resolution.
    By contrast, in OpenSCAD, curved surfaces are represented by polygonal
    approximations that are chosen when the object is created, and errors accumulate as
    these curved surfaces are further transformed. In OpenSCAD, you must find
    a value of `$fn` that's high enough to achieve the print quality you want
    while still low enough to prevent OpenSCAD from getting too slow or crashing while
    you are working on the design.

* **complex objects with micro-fine detail** <br>
  Using Functional Geometry, it is possible to create models that have huge
  amounts of procedurally generated detail, without making preview too slow
  or exceeding your memory limit. For example, fractals or digital fabrics.
  You can create complex models that would be impossible in OpenSCAD, because
  too many triangles would be required. To accomplish this, you need to use
  either the (new) high level spatial repetition and patterning operators, or the low level API,
  since unioning a million objects is just as problematic in FG as it is in OpenSCAD.

* **no "non-manifold objects"** <br>
  The OpenSCAD rendering engine, based on CGAL, is very picky about "non-manifold objects",
  so you have to use tricks to perturb your model in ways to avoid these errors.
  The problem doesn't occur in preview mode, and it's not something you worry about
  in Functional Geometry either.

* **high level annoyances** <br>
  Of course, Functional Geometry has its own limitations.
  We will discover that the FG high level API has its own annoyances that affect
  the programming model.

* **low level annoyances** <br>
  The FG low level API has an ease of use problem, similar to `polyhedron` in OpenSCAD.
  It's possible to write a bad distance function for a primitive shape,
  which could cause error messages or rendering problems later, and it's hard to
  automatically detect and report these problems when the shape is constructed.

* **no `minkowski` or `hull`** <br>
  There's no efficient implementation of Minkowski Sum and Convex Hull.
  Of course, we could convert functional representation to a mesh, and run the
  mesh versions of these operations, but that's even slower than OpenSCAD.
  Even in OpenSCAD, these are slow operations that kill preview performance.
  Fortunately, there are good alternatives to the OpenSCAD idioms that
  use these operations, which preview quickly.

* **not a boundary representation** <br>
  Functional representation does not directly represent the boundary of an object in the
  same way that a mesh does. This may lead to compromises when what you really want is
  direct control over the mesh, for example in STL export. It may require you to
  make additional decisions to configure space/time/accuracy tradeoffs during mesh generation.
  If necessary, I'll investigate a hybrid approach to mitigate this problem,
  but using mesh features will take away from the simplicity of Functional Geometry programming.

## Functional Representation (F-Rep)

F-Rep (functional representation) is a leading alternative to B-Rep (boundary representation)
as a general purpose representation for geometric objects. Examples of B-Rep include the
OpenSCAD polyhedral mesh, and spline based representations.
Like voxel representations, F-Rep is considered a volumetric representation.

In F-Rep, a shape is represented by a distance field:
a scalar field that specifies the minimum distance to a shape.
The distance is signed: negative inside the shape, zero on the boundary,
and positive outside the shape.

A distance field is a function that maps every point in space (specified as [x,y,z])
to a distance value. A distance function can be derived from the mathematical equation
for a shape. For example, a sphere of radius `r` has the equation
```
x^2 + y^2 + z^2 = r^2
```
This equation is true for all [x,y,z] points on the surface of the sphere.

We can rewrite this as `x^2 + y^2 + z^2 - r^2 = 0`,
and then derive a scalar field
```
f[x,y,z] = x^2 + y^2 + z^2 - r^2
```
which has the value zero for points on the surface of the sphere.

To convert this to a distance field, we need one more transformation:
```
f[x,y,z] = sqrt(x^2 + y^2 + z^2) - r
```

### Constraints on Distance Functions
We need to constrain distance functions so that the algorithms
used to render shapes and export them to other data structures
will still work.

A distance function returns a *minimum distance* `d`
between the point argument and the nearest object boundary.
* If `d` is 0,
  then the point must be on an object boundary.
* Otherwise, the nearest object boundary
  must be at least `abs(d)` units away,
  but it could be farther. There is room for flexibility
  because of isosurfaces, discussed below.
* If this constraint is violated, then the ray-marching algorithm
  for directly rendering a shape on a GPU will not work.

A distance function must be [strictly increasing](http://mathworld.wolfram.com/StrictlyIncreasingFunction.html)
(for points outside of objects) or strictly decreasing
(for points inside of objects) as the point gets
increasingly farther away from the nearest object boundary.
This paragraph is speculative.

### Isosurfaces
The isosurface at distance d of a distance field f
is the set of all points p such that f(p)=d.
The isosurface at distance 0 is the boundary of the shape.
The isosurface at distance 1 is another boundary, a shell
that encloses the shape, and is separated from it by a minimum distance of 1.
Isosurfaces are important because they are used to compute shells.

When you design a distance function, you aren't just specifying
the boundary of the shape, you are also specifying an infinite family
of isosurfaces that enclose or are enclosed by the shape.
It's important to consider what all of these isosurfaces look like,
because that will determine what shells look like, as computed by
the `shell` and `inflate` operations.

### Infinite Shapes
Distance fields are not restricted to representing finite geometrical objects.
They can also represent infinite space-filling patterns.
For examples, try a Google image search on
[k3dsurf periodic lattice](https://www.google.ca/search?q=k3dsurf+periodic+lattice&tbm=isch).
These infinite patterns are useful in 3D modelling:
you can intersect them or subtract them from a finite 3D object.

### Materials and Colours
A material field or a colour field can be associated with a shape,
assigning a material or colour to every point within the volume of a shape.
This is an aspect of F-Rep that will be added to FG in a later revision.

### Other Resources

This topic is hard to google, because so many different terms are used.
Try F-Rep, FRep, functional representation,
On the Representation of Shapes Using Implicit Functions,
distance field, distance function, signed distance function,
isosurface, procedural modeling.

Try this essay on
[the mathematical basis of F-Rep](https://christopherolah.wordpress.com/2011/11/06/manipulation-of-implicit-functions-with-an-eye-on-cad/)
by Christopher Olah, inventor of ImplicitCAD.

## The Representation of Shapes in FG
In the Functional Geometry system, every shape has two fields:
* `dist`, a distance function.
* `bbox`, a function which maps a distance value `d`
  onto the bounding box of the isosurface of `d`.

A bounding box has the representation `[[xmin,ymin,zmin],[xmax,ymax,zmax]]`.
A bounding box may be larger than is strictly necessary to enclose the
isosurface, but it must not be smaller. This flexibility is permitted
due to the difficulty in computing an exact bounding box for some shapes.

Empty shapes are supported, but the bounding box must also be empty.
A bounding box has zero width along the X axis if xmin >= xmax,
which means the bounding box is empty. Likewise for the Y and Z axes.

A bbox function should handle a distance argument outside the range of
its associated dist function by returning the empty bounding box.
This can happen when computing shells. If you are lucky, then an out of range
distance argument will result in your bbox function returning a bounding box
where xmin > xmax, or ymin > ymax, or zmin > zmax, just because the arithmetic
works out that way, and not because you wrote special case code.

Both finite and infinite shapes are supported.
If a shape extends to infinity along any of the 6 cardinal directions,
then this is indicated in the bounding box by setting xmin, ymin or zmin
to -inf, or by setting xmax, ymax or zmax to inf.

A distance field can, in principle, specify a degenerate shape that has zero thickness
across some or all of its extension. Which classes of degenerate shapes that we
actually support is an open question. We use 64 bit IEEE floats for geometry
computations, so shapes could become degenerate as a result of floating point underflow.
This suggests that the FG geometry engine should be very tolerant of degeneracy.

### Rationale
Why do we need to specify a bounding box?
Because the geometry kernel needs it to render STL, and to pick a bounding box
for the preview window.

There are two kinds of F-Rep geometry systems:
* Systems like Hyperfun or ShapeJS require the user to specify a global bounding
  box for all the geometry in a project. But primitive operations can be coded without
  specifying a bounding box, since that's the user's problem.
* Systems like ImplicitCAD, Antimony and FG automatically compute a bounding box
  for each shape and operator, so that the user doesn't have to, but this moves the
  complexity into the definition of each primitive.

Why do we need to specify a bounding box *function*, instead of just a single
bounding box for the isosurface at value 0?
Because it's required by `inflate`, and other operations that compute shells.
In the general case, you can't predict the bounding box of a shell from
the bounding box of the isosurface at 0. A few primitives
have "weird" isosurfaces with unusual bounding boxes, including
anisotropic `scale`.

## Low Level API

`3dshape(dist([x,y,z])=..., bbox(d)=...)`
> Returns a functional 3D shape, specified by a distance function
> and a bounding box function.

`2dshape(dist([x,y])=..., bbox(d)=...)`
> Returns a functional 2D shape, specified by a distance function
> and a bounding box function.

The low level API contains utility operations that are used
to define new operations that map shapes onto shapes.
Shapes can be queried at run-time for their distance
and bounding box functions.

`shape.dist`
> the distance function of a shape

`shape.bbox`
> the bounding box function of a shape

## Circle and Sphere
A circle of radius `r`, centred on the origin:
```
circle(r) = 2dshape(
  dist(p) = norm(p) - r,
  bbox(d)=[[-r,-r]-d,[r,r]+d]);
```
The `dist` function is derived from
[the mathematical equation for a circle](https://en.wikipedia.org/wiki/Circle#Equations):
```
   x^2 + y^2 = r^2
-> sqrt(x^2 + y^2) = r
-> sqrt(x^2 + y^2) - r = 0
-> dist([x,y]) = sqrt(x^2 + y^2) - r
-> dist(p) = norm(p) - r
```
This particular `dist` function returns the shortest euclidean distance
from the point to the perimeter of the circle.

Every `dist` function defines an infinite family of isosurfaces,
or in the 2D case, isolines.
For this particular `dist` function, all of the isolines are circles,
so the shell of a circle is another circle.

Most primitive operations, including `circle`, have isosurfaces that behave in a standard way:
the bbox expands outwards by 1 unit for each unit increase in d.
For these operations, the bbox function has the form:
```
bbox(d) = [[...]-d, [...]+d]
```

The range of our `dist` function is infinity to `-r`.

A `bbox` function is required to return the empty bbox
if it is passed an argument outside the range of the `dist` function.
In this case, the math works out so that this happens automatically,
without using an `if` expression. Eg,
```
  c = circle(2);
  c.bbox(-3) == [[1,1],[-1,-1]]
```
And this bbox is empty since xmin > xmax, and ymin > ymax.

The definition of `sphere` is very similar:
```
sphere(r) = 3dshape(
  dist(p) = norm(p) - r,
  bbox(d)=[[-r,-r,-r]-d,[r,r,r]+d]);
```

## CSG Operations
```
union(s1,s2) = 3dshape(
  dist(p) = min(s1.dist(p), s2.dist(p)),
  bbox(d)=[ min(s1.bbox(d)[0],s2.bbox(d)[0]), max(s1.bbox(d)[1],s2.bbox(d)[1]) ]);
```
```
intersection(s1,s2) = 3dshape(
  dist(p) = max(s1.dist(p), s2.dist(p)),
  bbox(d)=[ max(s1.bbox(d)[0],s2.bbox(d)[0]), min(s1.bbox(d)[1],s2.bbox(d)[1]) ]);
```
Union and intersection are implemented as the minimum and maximum
of the shape argument distance functions. This computes the union or
intersection of all of the isosurfaces in the distance functions.
Whenever you see `min` or `max` of a distance, in a distance function,
a union or intersection is being computed.
Also, it's likely that a sharp angle is being introduced into the shape.

```
complement(s) = 3dshape(
  dist(p) = -s.dist(p),
  bbox(d) = [[-inf,-inf,-inf],[inf,inf,inf]] );
```
The `complement` operation reverses the inside and outside of a shape.
The result is infinite.
It's useful when working with infinite space filling patterns,
and for defining `difference`.

```
difference(a,b) = intersection(a, complement(b));
```
Subtract `b` from `a`.

## Rectangle and Cuboid

Consider an axis-aligned rectangle, specified by `[[xmin,ymin],[xmax,ymax]]`.
This is a convex 4-sided polygon, which we can compute by
[intersecting 4 half-planes](https://en.wikipedia.org/wiki/Convex_polytope#Intersection_of_half-spaces).

Consider the half-plane containing all points such that x >= xmin.
Rewrite this as xmin <= x.
The corresponding distance function is `xmin - x`.

Here are the 4 half-planes, and their distance functions:
```
x >= xmin    xmin - x
x <= xmax    x - xmax
y >= ymin    ymin - y
y <= ymax    y - ymax
```
Here is their intersection:
```
max(xmin - x, x - xmax, ymin - y, y - ymax)
```
And here's the code:
```
box2([[xmin,ymin],[xmax,ymax]]) = 2dshape(
  dist([x,y]) = max(xmin - x, x - xmax, ymin - y, y - ymax),
  bbox(d)=[ [xmin,ymin]-d, [xmax,ymax]+d ]);
```

Now let's design a centred rectangle primitive, where the width and height are specified by `[dx,dy]`.

The distance function is `max(-dx/2 - x, x - dx/2, -dy/2 - y, y - dy/2)`.

We can simplify and speed it up: `max(abs(x) - dx/2, abs(y) - dy/2)`,
exploiting the fact that the distance is symmetric around the origin.

Here's the code:
```
rectangle([dx,dy]) = 2dshape(
  dist([x,y,z]) = max(abs(x) - dx/2, abs(y) - dy/2),
  bbox(d)=[ [-dx/2,-dy/2]-d, [dx/2,dy/2]+d ]);
```

Cuboid is similar; conceptually it works by
[intersecting 6 half-spaces to make the 6 faces of the cuboid](https://en.wikipedia.org/wiki/Convex_polytope#Intersection_of_half-spaces):
```
cuboid([dx,dy,dz]) = 3dshape(
  dist([x,y,z]) = max(abs(x)-dx/2, abs(y)-dy/2, abs(z)-dz/2),
  bbox(d)=[ [-dx/2,-dy/2,-dz/2]-d, [dx/2,dy/2,dz/2]+d ]);
```

This technique can be generalized to model any convex polygon/polyhedron,
by intersecting one half-plane/half-space for each edge/face.
It can be further generalized to non-convex polytopes by using
[Nef polygons/polyhedra](https://en.wikipedia.org/wiki/Nef_polygon),
where both intersection and complement operations are used.

However, this technique requires each edge/face to be examined in each
call to the distance function. It will become inefficient for sufficently large
numbers of edges/faces; for that, another approach should be found.

## Cylinder and Linear_extrude

We will construct a centred cylinder by intersecting 3 infinite shapes:
two half-spaces (one for each end cap), and an infinite cylinder
for the body. This is similar to the construction of a cuboid
from six half-spaces.

We'll first construct an infinite cylinder centered on the Z-axis.
To do this, we just take the distance function for a centered circle,
and put that into a 3D distance function:
```
dist([x,y,z]) = norm([x,y]) - r
```
We get infinite extension along the Z axis by ignoring the `z` parameter.
This way, every slice through the distance field that is parallel to the X-Y plane
looks exactly the same.

The two half-spaces follow from our construction for cuboid.
```
z <= h/2    z - h/2
z >= -h/2   -h/2 - z
```
The intersection of these three infinite solids is
```
max(norm([x,y]) - r, z - h/2, -h/2 - z)
```
We'll apply the same optimization for a centered object that we used for cuboid:
```
max(norm([x,y]) - r, abs(z) - h/2)
```
And here's the code:
```
cylinder(h,r) = 3dshape(
  dist([x,y,z]) = max(norm([x,y]) - r, abs(z) - h/2),
  bbox(d)=[ [-r,-r,-h/2]-d, [r,r,h/2]+d ]);
```
This can be generalized to `linear_extrude`:
```
linear_extrude(h)(s) = 3dshape(
  dist([x,y,z]) = max(s.dist([x,y]), abs(z) - h/2),
  bbox(d)=[ [s.bbox(d)[0].x, s.bbox(d)[0].y, -h/2-d],
            [s.bbox(d)[1].x, s.bbox(d)[1].y, h/2+d] ]);
```
Now cylinder can be defined like this:
```
cylinder(h,r) = linear_extrude(h) circle(r);
```

## Rotate_extrude

(Torus from Olah's essay):
We can use the output of one of these functions as the input for another. For example, if we feed r = \sqrt{x^2+y^2} - k_1, the outwardness of a circle on the x,y-plane, into a new circle which we can think of as being on the r,z-plane, \sqrt{r^2+z^2}-k_2, we get a torus.
That is, \sqrt{(\sqrt{x^2+y^2} - k_1)^2+z^2}-k_2 = 0 results in a torus.


rotate_extrude from ImplicitCAD
```
getImplicit3 (RotateExtrude totalRotation round translate rotate symbObj) = 
    let
        tau = 2 * pi
        k   = tau / 360
        totalRotation' = totalRotation*k
        obj = getImplicit2 symbObj
        capped = Maybe.isJust round
        round' = Maybe.fromMaybe 0 round
        translate' :: ℝ -> ℝ2
        translate' = Either.either 
                (\(a,b) -> \θ -> (a*θ/totalRotation', b*θ/totalRotation')) 
                (. (/k))
                translate
        rotate' :: ℝ -> ℝ
        rotate' = Either.either 
                (\t -> \θ -> t*θ/totalRotation' ) 
                (. (/k))
                rotate
        twists = case rotate of
                   Left 0  -> True
                   _       -> False
    in
        \(x,y,z) -> minimum $ do
            
            let 
                r = sqrt (x^2 + y^2)
                θ = atan2 y x
                ns :: [Int]
                ns =
                    if capped
                    then -- we will cap a different way, but want leeway to keep the function cont
                        [-1 .. (ceiling (totalRotation'  / tau) :: Int) + (1 :: Int)]
                    else
                        [0 .. floor $ (totalRotation' - θ) /tau]
            n <- ns
            let
                θvirt = fromIntegral n * tau + θ
                (rshift, zshift) = translate' θvirt 
                twist = rotate' θvirt
                rz_pos = if twists 
                        then let 
                            (c,s) = (cos(twist*k), sin(twist*k))
                            (r',z') = (r-rshift, z-zshift)
                        in
                            (c*r' - s*z', c*z' + s*r')
                        else (r - rshift, z - zshift)
            return $
                if capped
                then MathUtil.rmax round' 
                    (abs (θvirt - (totalRotation' / 2)) - (totalRotation' / 2))
                    (obj rz_pos)
                else obj rz_pos
```

ExtrudeOnEdgeOf from ImplicitCAD
```
getImplicit3 (ExtrudeOnEdgeOf symbObj1 symbObj2) =
    let
        obj1 = getImplicit2 symbObj1
        obj2 = getImplicit2 symbObj2
    in
        \(x,y,z) -> obj1 (obj2 (x,y), z)
getBox3 (ExtrudeOnEdgeOf symbObj1 symbObj2) =
    let
        ((ax1,ay1),(ax2,ay2)) = getBox2 symbObj1
        ((bx1,by1),(bx2,by2)) = getBox2 symbObj2
    in
        ((bx1+ax1, by1+ax1, ay1), (bx2+ax2, by2+ax2, ay2))
```
```
perimeter_extrude(a,b) = 3dshape(
  dist([x,y,z]) = a.dist([b.dist([x,y]), z]),
  bbox(d)=let ([[ax1,ay1],[ax2,ay2]] = a.bbox(d),
               [[bx1,by1],[bx2,by2]] = b.bbox(d))
          [[bx1+ax1, by1+ax1, ay1], [bx2+ax2, by2+ax2, ay2]] );
```
Extrude 2D object `a` around the perimeter of 2D object `b` to get a 3D object.
The perimeter of 'b' lies on the X-Y plane. The (0,0) point of `a` is aligned with
the perimeter as `a` is swept around the perimeter.

If `a` and `b` are both circles, then you get a torus.

This is ExtrudeOnEdgeOf from ImplicitCAD.
Based on a few rendered examples, figure `a` is normal to the perimeter at each
point on the perimeter, which is what you want. But how does it work?

```
rotate_extrude(r)(s) = perimeter_extrude(s, circle(r));
torus(r1,r2) = rotate_extrude(r1) circle(r2);
```

## Shells

There are many different ways to write a distance function that produce the same visible result.
The main requirement is that the isosurface of the distance function at 0
(the set of all points such that f[x,y,z] == 0)
is the boundary of the desired object, and there are infinitely many functions that satisfy this
requirement for any given shape.

But the isosurface at 0 is not the only thing you need to consider.
A distance function defines an infinite family of isosurfaces,
and they are all important, particular when you use the `shell` primitive.
Consider
```
c = cuboid([10,20,30]);
```
`c.dist` is the distance function. Because of the way we defined `cuboid`,
the isosurface of `c.dist` at -1 is cuboid(8,18,28]), the isosurface at 1
is cuboid([12,22,32]), and so on. All of the isosurfaces of `c` are cuboids,
although with varying aspect ratios.
As a result, `shell(1) sp` will return a spherical shell with outer radius 10
and inner radius 9. The `shell` operator provides direct access to the other isosurfaces.
(See the Shell section, which is later.)

I have similar comments for `cuboid`.
The distance function is designed so that all of the isosurfaces of a given
cuboid are nested cuboids, and `shell` works as expected.

Here's a thought experiment. Consider defining the distance function for `cuboid`
so the the distance to the boundary is measured in millimeters along a vector that points to the centre.
With this definition, only the isosurface at 0 would be a true cuboid. The isosurfaces at positive values
would have increasingly convex sides, so that at large values, the surface would approach a sphere.
The isosurfaces at negative values would have increasingly concave sides.
This behaviour would be visible using `shell`.


## Primitive Shapes
Here I'll define 3 primitive shapes: sphere, cuboid and cylinder.
The main lesson will be learning how to write a distance function.

```
sphere(r) = 3dshape(
    f([x,y,z]) = sqrt(x^2 + y^2 + z^2) - r,
    bbox=[[-r,-r,-r],[r,r,r]]);
```

In the F-Rep section, I said that `f([x,y,z]) = x^2 + y^2 + z^2 - r^2`
is a distance function for a sphere, so why did we change it?

There are many different ways to write a distance function that produce the same visible result.
The main requirement is that the isosurface of the distance function at 0
(the set of all points such that f[x,y,z] == 0)
is the boundary of the desired object, and there are infinitely many functions that satisfy this
requirement for any given shape.

But the isosurface at 0 is not the only thing you need to consider.
A distance function defines an infinite family of isosurfaces,
and they are all important, particular when you use the `shell` primitive.
Consider
```
sp = sphere(10);
```
`sp.f` is the distance function. Because of the way we defined `sphere`,
the isosurface of `sp.f` at -1 is a sphere of radius 9, the isosurface at 1
is a sphere of radius 11, and so on. All of the isosurfaces of `sp` are spheres.
As a result, `shell(1) sp` will return a spherical shell with outer radius 10
and inner radius 9. The `shell` operator provides direct access to the other isosurfaces.
(See the Shell section, which is later.)

```
cuboid(sz) = 3dshape(
  f([x,y,z]) = max([abs(x-sz.x/2), abs(y-sz.y/2), abs(z-sz.z/2)]),
  bbox=[ [0,0,0], sz ]);
```
I have similar comments for `cuboid`.
The distance function is designed so that all of the isosurfaces of a given
cuboid are nested cuboids, and `shell` works as expected.

Here's a thought experiment. Consider defining the distance function for `cuboid`
so the the distance to the boundary is measured in millimeters along a vector that points to the centre.
With this definition, only the isosurface at 0 would be a true cuboid. The isosurfaces at positive values
would have increasingly convex sides, so that at large values, the surface would approach a sphere.
The isosurfaces at negative values would have increasingly concave sides.
This behaviour would be visible using `shell`.

Note the use of `max` in the distance function for `cuboid`.
Generally speaking, if your distance function computes the max of a function of `x` and a function of `y`,
then your shape will have a right angle between the X and Y axes.
Read [Olah's essay](https://christopherolah.wordpress.com/2011/11/06/manipulation-of-implicit-functions-with-an-eye-on-cad/) for more discussion of this.

```
cylinder(h,r) = linear_extrude(h=h) circle(r);
```

```
circle(r) = 2dshape(
    f([x,y]) = sqrt(x^2 + y^2) - r,
    bbox=[[-r,-r],[r,r]]);
```

```
linear_extrude(h)(shape) = 3dshape(
    f([x,y,z]) = max(shape.f(x,y), abs(z - h/2) - h/2),
    bbox=[ [shape.bbox[0].x,shape.bbox[0].y,-h/2], [shape.bbox[1].x,shape.bbox[1].y,h/2] ]);
```
> The result of `linear_extrude` is centred.

The distance function for `linear_extrude` computes the max
of a function of x & y with a function of z,
and the result is a right angle between the X/Y plane and the Z axis.
See above.

## Translation
translate(), align(), pack???.
No need for center= options.

## Shells and Isosurfaces

### `shell(n) shape`
hollows out the specified shape, leaving only a shell of the specified
thickness.
An analogy is 3D printing a shape without infill.

```
shell(w)(shape) = 3dshape(
  f(p) = abs(shape.f(p) + w/2) - w/2,
  bbox = shape.bbox );
```
Explanation:
* `f(p) = abs(shape.f(p))` is the zero-thickness shell of `shape`.
* `f(p) = abs(shape.f(p)) - n` (for positive n) is a shell of thickness `n*2`,
  centered on the zero-thickness shell given above.
* `f(p) = shape.f(p) + n` (for positive n) is the isosurface of `shape` at `-n`
  (it's smaller than `shape`)

### `inflate(n) shape`
returns the isosurface of `shape` at value `n`.
Positive values of `n` create a larger version of `shape`,
`n==0` is the identity transformation,
and negative values "deflate" the shape.
This is different from the scale transformation; it's more like
inflating a shape like a balloon, especially for non-convex objects.

![inflate](img/inflate.png)
<br>[image source](http://www.gpucomputing.net/sites/default/files/papers/2452/Bastos_GPU_ADF_SMI08.pdf)

```
inflate(n)(shape) = 3dshape(
  f(p) = shape.f(p) - n,
  bbox=[ shape.bbox[0]-n, shape.bbox[1]+n ]);
```
TODO: is the bbox calculation correct in all cases? No, not for anisotropic scaling.
The bbox could be bigger than this.
Use f to compute the bbox from shape.bbox using ray-marching.

## Scaling
Scaling is tricky. Several functional geometry systems get it wrong?

### `isoscale(s) shape`
This is an isotropic scaling operation:
it scales a shape by the same factor `s` on all 3 axes,
where `s` is a positive number.
The code is easier to understand than for anisotropic scaling.

```
isoscale(s)(shape) = 3dshape(
  f(p) = s * shape.f(p / s),
  bbox = s * shape.bbox
);
```

Suppose that the distance function
was instead `f(p)=shape.f(p/s)`,
as it is in Antimony. We don't multiply the result by `s`.
This is good enough to scale the isosurface at 0,
which means the scaled shape will render correctly.
But the other isosurfaces will be messed up. Why?
* Suppose s > 1. Eg, s==2 and we are scaling a centred sphere with radius 3.
  We want the result to be a sphere with radius 6.
  If we pass in p=[6,0,0], that's converted to p/s = [3,0,0] before passed to the sphere's distance function,
  which then returns 0, indicating that p is on the boundary. Good.
  If we pass in p=[8,0,0], that's converted to p/s = [4,0,0], then shape.f(p/s) returns 1.
  This indicates we are a minimum of 1 mm from the boundary, which is satisfies
  the ray-march property. However, the inflate property is not satisfied, since the
  bounding box returned by `inflate(1)scale(2)sphere(3)` will be too small.
* Suppose s < 1.
  The ray-march property fails, but the inflate property is satisfied.
  

### Negative Scale Factor
In OpenSCAD, a negative scaling factor passed to the scale() operator
will cause a reflection about the corresponding axis. This isn't documented,
but it is a natural consequence of how scaling is implemented by an affine transformation matrix.

The scale operators discussed in this document don't work that way: a negative scaling factor
results in garbage.

### `scale([sx,sy,sz]) shape`
Anisotropic scaling is hard ...

I don't see a way to implement anisotropic scaling in a way
that satisfies both the ray-march and the inflate properties.
The system needs to change.

the effect of anisotropic scaling on `shell`. Need for `spheroid`.

```
getImplicit3 (Scale3 s@(sx,sy,sz) symbObj) =
    let
        obj = getImplicit3 symbObj
        k = (sx*sy*sz)**(1/3)
    in
        \p -> k * obj (p ⋯/ s)
getBox3 (Scale3 s symbObj) =
    let
        (a,b) = getBox3 symbObj
    in
        (s ⋯* a, s ⋯* b)
```

```
scale(s)(shape) = 3dshape(
  f(p) = (s.x*s.y*s.z)^(1/3) * shape.f(p / s),
  bbox = s * shape.bbox
);
```

explanation:
* `f0(p) = shape.f(p/s)` correctly scales the isosurface at 0,
  so the shape will render correctly.
  But the other isosurfaces will be messed up,
  because the distances between isosurfaces will also be scaled.
  So `f0` isn't a valid distance function:
  it will cause `shell` and `inflate` to not work, in some cases.
* ImplicitCAD multiplies f0 by k=cube_root(s.x * s.y * s.z)
  to get a valid distance function. I don't understand.
  Suppose s=[27,1,1]. Then k=3. How can this be correct?
* Antimony just uses `f0`; it doesn't even try to fix the other
  isosurfaces. I don't think the correct code is even expressible.

Antimony:
```
def scale_x(part, x0, sx):
    # X' = x0 + (X-x0)/sx
    return part.map(Transform(
        '+f%(x0)g/-Xf%(x0)gf%(sx)g' % locals()
                if x0 else '/Xf%g' % sx,
        'Y',
        '+f%(x0)g*f%(sx)g-Xf%(x0)g' % locals()
                if x0 else '*Xf%g' % sx,
        'Y'))
def scale_xyz(part, x0, y0, z0, sx, sy, sz):
   # X' = x0 + (X-x0)/sx
   # Y' = y0 + (Y-y0)/sy
   # Z' = z0 + (Z-z0)/sz
   # X = x0 + (X'-x0)*sx
   # Y = y0 + (Y'-y0)*sy
   # Z = z0 + (Z'-z0)*sz
   return part.map(Transform(
      '+f%(x0)g/-Xf%(x0)gf%(sx)g' % locals(),
      '+f%(y0)g/-Yf%(y0)gf%(sy)g' % locals(),
      '+f%(z0)g/-Zf%(z0)gf%(sz)g' % locals(),
      '+f%(x0)g*-Xf%(x0)gf%(sx)g' % locals(),
      '+f%(y0)g*-Yf%(y0)gf%(sy)g' % locals(),
      '+f%(z0)g*-Zf%(z0)gf%(sz)g' % locals()))
Shape Shape::map(Transform t) const
{
    return Shape("m" + (t.x_forward.length() ? t.x_forward : "_")
                     + (t.y_forward.length() ? t.y_forward : "_")
                     + (t.z_forward.length() ? t.z_forward : "_") + math,
                 bounds.map(t));
}
```
The scale functions also do translation, so you can scale an object locally without moving its origin point.
With the translation removed, we get
```
   # X' = X/sx
   # X = X'*sx
```
Antimony gets it wrong. It doesn't fix up the

## CSG Operations
everything, nothing, complement,
union, intersection, difference

```
complement(s) = 3dshape(
  f(p) = -s.f(p)
  bbox = [[-inf,-inf,-inf],[inf,inf,inf]] );
```
> Convert a shape or pattern to its inverse: all points inside the object
> are now outside, and vice versa.

meaning of 'max' and 'min' in a distance function

Note: ImplicitCAD and Antimony use max for union and min for intersection.
Although this is a common implementation, [Schmitt 2002]
(http://grenet.drimm.u-bordeaux1.fr/pdf/2002/SCHMITT_BENJAMIN_2002.pdf)
says "it is well known that using min/max functions for set-theoretic
operations causes problems in further transformations of the model due to C1 discontinuity of
the resulting function (see Fig. 1.3). Further blending, offsetting, or metamorphosis can result
in unnecessary creases, edges, and tearing of such an object." (page 26, and figure 1.3 on page 20).
His solution is to use an alternative function, one which is differentiable.
My interpretation is that you need to round any sharp edges and add fillets to sharp corners
to avoid this alleged problem. I'm going to wait and see if we can actually detect a problem
once we have working code, before worrying about a fix.

## Rounded edges and Fillets
rcuboid, runion

```
rcuboid(r,sz) = 3dshape(
  f([x,y,z]) = rmax(r, [abs(x-sz.x/2), abs(y-sz.y/2), abs(z-sz.z/2)]),
  bbox=[ [0,0,0], sz ]);
```
> Cuboid with rounded corners (radius=r) from ImplicitCAD.
> All ImplicitCAD primitives that generate sharp corners have a rounding option.
> `rmax` is a utility function for creating rounded corners.

```
runion(r,s1,s2) = 3dshape(
    f(p) = rmin(r, s1.f(p), s2.f(p)),
    bbox=[ min(s1.bbox[0],s2.bbox[0])-r, max(s1.bbox[1],s2.bbox[1])+r ]);
```
> Create fillets! ImplicitCAD rounded union with radius `r`. (Why is the bbox padded by r?)
Example:
  ```
  runion(r=8,
  	cube([40,40,20]),
  	translate([0,0,20]) cube([20,20,30]))
  ```
![rounded union](http://thingiverse-rerender-new.s3.amazonaws.com/renders/47/9e/1c/c8/e2/RoundedUnionCubeExample_preview_featured.jpg)

## Perimeter_extrude

## Infinite Patterns
gyroid, infinite replication grid, etc

## Morphing
"Distance fields are also commonly used for
shape metamorphosis. Compared to mesh-based morphing
techniques, volume-based morphing is easier to implement
and can naturally morph surfaces of different genus."
[[Bastos 2008](http://www.gpucomputing.net/sites/default/files/papers/2452/Bastos_GPU_ADF_SMI08.pdf)].

Here's a simple morph operator based on linear interpolation
of the distance functions:

![morph](img/morph.png)
<br>[image source](http://www.gpucomputing.net/sites/default/files/papers/2452/Bastos_GPU_ADF_SMI08.pdf)

```
morph(r,s1,s2) = 3dshape(
  f(p) = interp(r, s1.f(p), s2,f(p)),
  bbox = interp(r, s1.bbox, s2.bbox)
);
interp(r,a,b) = a + (b - a)*r;
```
`r` normally ranges from 0 to 1, where 0 returns s1, 1 returns s2.
But values <0 or >1 can also be used.

"Unfortunately, in most cases simple interpolation is
unable to provide convincing metamorphoses, as it does not
preserve or blend corresponding shape features. In order to
improve morphing quality, we could use a warp function
to guide the distance field interpolation, as proposed by
Cohen-Or 'Three dimensional distance field metamorphosis'"
[[Bastos 2008](http://www.gpucomputing.net/sites/default/files/papers/2452/Bastos_GPU_ADF_SMI08.pdf)].
