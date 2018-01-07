===============
The Shape Atlas
===============
The Shape Atlas is a catalog of operations for constructing,
transforming and combining 2D and 3D geometric shapes.

This is a research project to identify, classify and implement
all of the artistically useful operations on signed distance fields
(the representation Curv uses for geometric shapes).

Each operation will be given a provisional API, and sometimes we'll explore multiple
implementations with different tradeoffs.

The ultimate goal is to boil all of this research down into a well
designed, consistent and powerful geometry API for Curv, which will be
included in a future Curv standard library.

The Standard Shape Library
==========================
At this point, the Shape Atlas is well enough developed for me to speculate
on the structure of the Curv standard shape library.

The goal is to be easy to use for beginners, but powerful enough for experts.
Some shape operations require a lot more knowledge and expertise to use than others,
so we'll divide the shape library into levels.

Level 0: Basic Shape API
  Includes the shape constructors, the boolean operations, and the transformations (sections 1, 2 and 3).
  You don't need to know about signed distance fields to use this level.

Level 1: Advanced Shape API
  Includes the Distance Field Operations (section 4).
  You need to understand the different SDF classes, and which operations
  produce which classes of SDF.

Level 2: Expert Shape API
  Includes the ``make_shape`` function.
  You need to understand how define a new SDF from scratch.

I want to mess with the APIs to make them more composeable.

I want to try using keyword parameters where it makes sense.
OpenSCAD does this, and the Swift standard library is a particularly nice example.

0. Shape Properties
===================

2D and 3D Shapes
----------------
Every shape is marked as being 2-dimensional, 3-dimensional, or both.
2D shapes are embedded in the XY plane.
(The only standard shapes that are both are ``everything`` and ``nothing``.)

Infinite and Degenerate Shapes
------------------------------
A shape can be infinite. Many shape constructors accept ``inf`` as a dimension argument.

A 2D shape with no area, or a 3D shape with no volume, is called degenerate.
Examples are geometric points, line segments or curves, and in 3D, surfaces with 0 thickness.

Points and curves are invisible in the preview window, while surfaces are visible.
Infinite and degenerate shapes are useful as intermediates for constructing
shapes, even though you can't 3D print them or export them to some file formats.

Colour
------
Shapes have volumetric colour.
A function assigns a colour to each point in the interior and on the boundary
of every 2D and 3D shape. Primitive shapes are assigned a default yellowish colour,
which you change using the ``colour`` function.
Shape operations must specify how the colour of the result shape derives from the
colour of the argument shapes.
(Colour assignment and colour transformation operators will be researched elsewhere,
in `<Colour_Atlas.rst>`_.)

Signed Distance Fields
----------------------
Shapes are represented internally as Signed Distance Fields (SDFs), see `<Theory.rst>`_.
This is not a unique representation: a given shape can be represented by many different SDFs.
In most cases, the user just wants to let the implementation choose an SDF that is fast
to compute and fast to render.

However, there are Distance Field Operations (see section 4)
that construct a shape based on the SDF of an input shape.
These operations are too useful to leave out.
For this reason, we document the class of SDF created by each shape operation.

These are the SDF classes:

exact:
  The distance field contains the exact Euclidean distance to the nearest boundary.
  The ``offset`` operation will create a rounded offset.
mitred:
  Like exact, except vertex and edge information is preserved in all isosurfaces.
  This can be useful in conjunction with distance field operations.
  The ``offset`` operation will create a mitred offset.
approximate:
  The SDF is implementation dependent, and may change between releases
  as the library is optimized.
bad:
  Worse than approximate: the SDF is Lipschitz continuous with a Lipschitz constant > 1.
  Sphere tracing won't work unless you correct the SDF using the ``lipschitz`` operator.
  The correction factor needs to be determined experimentally by the user.
discontinuous:
  Worse than bad: the SDF is not Lipschitz continuous, and can't be corrected by the ``lipschitz`` operator.
  This situation can occur when experimenting with ``make_shape``.

Bounding Box
------------
Each shape has an axis aligned bounding box, which may be either exact or approximate.
An approximate bounding box is larger than necessary to contain the shape.

All of the shape constructors create exact bounding boxes.
Some of the shape combinators produce exact bounding boxes if their input is exact,
(as documented), but many create approximate bounding boxes.

You need to worry about whether a bounding box is approximate or exact
if you use a shape combinator that uses the bounding box of its input
to determine the shape of its output.

1. Shape Constructors
=====================

2D Shapes
---------
``circle``
  Construct a circle centred on the origin. Exact distance field.

  * ``circle``: A prototypical circle of diameter 2 (ie, the unit circle).
  * ``circle d``: Construct a circle of diameter ``d``.

``ellipse (dx, dy)``
  Construct an axis-aligned ellipse, centred on the origin,
  with width ``dx`` and height ``dy``.
  Approximate distance field.
  
  * ``ellipse_e``: exact distance field, much more expensive to compute (TODO).

``square``
  Construct an axis-aligned square, centred on the origin.

  * ``square``: A prototypical square of width 2.
    I.e., it is inscribed by the unit circle.
  * ``square d``: Construct a square of width ``d``.
  * ``square.circumratio``: ratio of circumradius over inradius.
  * ``square_m d``: mitred distance field, simple code, cheap to compute.
  * ``square_e d``: exact distance field, more expensive.

``rect ...``
  Construct an axis-aligned rectangle.

  * ``rect (dx, dy)``: a rectangle of width ``dx`` and height ``dy``,
    centred on the origin. Either argument may be ``inf``.
  * ``rect ((xmin,ymin),(xmax,ymax))``: a rectangle with vertices
    at ``(xmin,ymin)`` and ``(xmax,ymax)``.
    ``xmin`` or ``ymin`` may be ``-inf``,
    and ``xmax`` or ``ymax`` may be ``inf``.
  * ``rect {xmin=-inf, ymin=-inf, xmax=inf, ymax=inf}``:
    Each argument specifies one of the sides, and defaults to ``-inf``
    or ``inf``. Used for specifying a clipping rectangle, with ``intersection``.
  * ``rect.mitred ...``: mitred distance field, cheaper to compute.
  * ``rect.exact ...``: exact distance field, more expensive.

``regular_polygon n``
  Construct a regular polygon, centred on the origin,
  with ``n`` sides, whose bottom edge is parallel to the X axis.
  Cost: constant time and space, regardless of ``n``.
 
  * ``regular_polygon n``: A prototypical instance,
    inscribed by the unit circle.
  * ``regular_polygon n d``: Construct a regular polygon
    whose inscribed circle has diameter ``d``.
  * ``regular_polygon n .circumratio``: Ratio of circumradius over inradius.
  * ``regular_polygon n .mitred d``: mitred distance field.
  * ``regular_polygon n .exact d``: exact distance field (TODO).

  TODO: Calls to regular_polygon should compile into optimized code,
  like http://thndl.com/square-shaped-shaders.html

..
  Example: ``regular_polygon 5``

..
  |pentagon|

.. |pentagon| image:: images/pentagon.png

``convex_polygon vertices``
  Construct a convex polygon from a list of vertices in counter-clockwise order.
  The result is undefined if the vertex list doesn't specify a convex polygon.
  Cost: linear in ``count(vertices)``.
 
  * ``convex_polygon_m``: mitred distance field.
  * ``convex_polygon_e``: exact distance field (TODO).

``polygon vertices``
  TODO. (Use the Nef Polygon construction, by combining a set of half-planes using intersection and complement.)

``stroke {d: diameter, from: point1, to: point2}``
  A line of thickness ``d`` drawn from ``point1`` to ``point2``,
  with semicircle end caps of radius ``d/2``.
  Exact distance field.

``half_plane ...``
  A half-plane is an infinite 2D shape consisting of all points on one side
  of an infinite straight line, and no points on the other side.
  The points on the line are included. Exact distance field.

  ``half_plane {d, normal: n}``
    A half-plane with normal vector ``n``,
    whose edge is distance ``d`` from the origin.
    ``n`` must be a unit vector.
    If d >= 0, the half-plane contains the origin.

  ``half_plane {at: p, normal: n}``
    A half-plane with normal vector ``n``,
    whose edge passes through point ``p``.
    ``n`` must be a unit vector.

  ``half_plane [p1, p2]``
    A half-plane whose edge passes through points p1 and p2.
    The normal vector is ``perp(p2-p1)``.
    Right hand palm on p1, fingertips on p2, thumb is the normal vector
    (points away from the half-plane).

3D Shapes
---------
``sphere``
  Construct a sphere centred on the origin. Exact distance field.

  * ``sphere``: A prototypical sphere of diameter 2 (ie, the unit sphere).
  * ``sphere d``: Construct a sphere of diameter ``d``.

``ellipsoid (dx, dy, dz)``
  Construct an axis-aligned ellipsoid, centred on the origin,
  with width ``dx``, depth ``dy`` and height ``dz``.
  Approximate distance field.
  
  * ``ellipsoid_e``: exact distance field, much more expensive to compute (TODO).

``cylinder {d, h}``
  Construct a cylinder, centered on the origin, whose axis of rotation is the Z axis.
  Diameter is ``d`` and height is ``h``.
 
  * ``cylinder_m``: mitred distance field.
  * ``cylinder_e``: exact distance field, more expensive.

``cone {d, h}``
  Construct a cone.
  The base (of diameter ``d``) is embedded in the XY plane and centred on the origin.
  The apex is above the origin at height ``h``.
 
  * ``cone_m``: mitred distance field.
  * ``cone_e``: exact distance field, more expensive.

``torus {major: d1, minor: d2}``
  Construct a torus, centred on the origin, axis of rotation is Z axis.
  Major diameter is ``d1`` (center of tube to centre of tube, crossing the origin).
  Minor diameter is ``d2`` (diameter of the tube).
  Total width of shape is ``d1+d2``.
  Exact distance field.

``box ...``
  Construct an axis-aligned cuboid.

  * ``box (dx, dy, dz)``: a cuboid of width ``dx``, depth ``dy``,
    and height ``dz``, centred on the origin. Any argument may be ``inf``.
  * ``box ((xmin,ymin,zmin),(xmax,ymax,zmax))``: a cuboid with vertices
    at ``(xmin,ymin,zmin)`` and ``(xmax,ymax,zmax)``.
    ``xmin``, ``ymin`` and ``zmin`` may be ``-inf``,
    ``xmax``, ``ymax`` and ``zmax``  may be ``inf``.
  * ``box {xmin=-inf, ymin=-inf, zmin=-inf, xmax=inf, ymax=inf, zmax=inf}``:
    Each argument specifies one of the faces, and defaults to ``-inf``
    or ``inf``. Used for specifying a clipping region, with ``intersection``.
  * ``box_m ...``: mitred distance field, cheaper to compute.
  * ``box_e ...``: exact distance field, more expensive.

``prism n ...``
  Construct a regular right prism, centred on the origin.
  The base is a regular polygon with ``n`` sides,
  parallel to the XY plane.

  ``prism n``
    The prism is, by default, sized so as to be inscribed by the unit sphere.
    Equivalent to ``prism n {d: 2, h: 2}``.
    Also, ``prism 4`` is equivalent to ``cube``.
  ``prism n {d, h}``
    The prism has height ``h``, and the base is inscribed by a circle of
    diameter ``d``.
  ``prism_m n {d, h}``
    With a mitred distance field.
  ``prism_e n {d, h}``
    With an exact distance field. (TODO)

``pyramid n {h, d}``
  TODO:
  Construct a regular right pyramid.
  The base is a regular polyhedron with ``n`` sides, whose inscribed circle has diameter ``d``.
  The base is embedded in the XY plane and centred on the origin.
  The apex is above the origin at height ``h``.
  Maybe provide an API for constructing an infinite pyramid with apex at origin?

Platonic Solids
  There are five definitions:

  * ``tetrahedron``
  * ``cube``
  * ``octahedron``
  * ``dodecahedron``
  * ``icosahedron``

  Each Platonic solid ``S`` has the following API:

  * ``S d`` constructs the solid centred on the origin whose
    inscribed sphere has diameter ``d``.
  * ``S`` is a prototypical instance of the solid, equivalent to ``S 2``
    (i.e., the inscribed sphere is the unit sphere with radius 1).
  * ``S.circumratio`` is the ratio of the circumradius over the inradius
    (a value > 1).
    For example,

    * ``S(d/S.circumratio)`` constructs an instance of S
      whose circumscribed sphere has diameter ``d``.
    * ``sphere(d*cube.circumratio)`` constructs a sphere that circumscribes
      a cube of height d.

  * ``S_m d`` constructs an instance of S with a mitred distance field.
  * ``S_e d`` constructs an instance of S with an exact distance field.

  TODO:

  * ``tetrahedron_e``
  * ``octahedron_e``
  * ``dodecahedron_e``
  * ``icosahedron_e``

``capsule {d: diameter, from: p1, to: p2}``
  A cylinder of diameter ``d`` whose central axis extends from ``p1`` to ``p2``,
  with the addition of hemispherical end caps of radius ``d/2``.
  Exact distance field.

``half_space ...``
  A half-space is an infinite 3D shape consisting of all points on one side
  of an infinite plane that subdivides 3-space. The points in the plane
  are included. Exact distance field.

  ``half_space {d, normal: n}``
    A half-space with normal vector ``n``,
    whose boundary plane is distance ``d`` from the origin.
    ``n`` must be a unit vector.
    If d >= 0, the half-space contains the origin.

  ``half_space {at: p, normal: n}``
    A half-space with normal vector ``n``,
    whose boundary plane passes through point ``p``.
    ``n`` must be a unit vector.

  ``half_space [p1, p2, p3]``
    A half-space whose boundary plane passes through points p1, p2 and p3.
    The points p1, p2 and p3 are in counter-clockwise order when viewed
    from above and outside of the half-space.

``gyroid``
  The gyroid surface (`<https://en.wikipedia.org/wiki/Gyroid>`_)
  is an infinite, labyrinthine, curved surface that is popular in 3D printed art.
  
  The gyroid surface partitions 3D space into two mirror image but congruent subspaces.
  The Curv ``gyroid`` constructor is one of these subspaces.
  You can get the other subspace using ``complement gyroid``,
  and you can get the gyroid surface using ``shell 0 gyroid``.
  
  TODO: distance field is bad.

Polydimensional Shapes
----------------------
``nothing``
  A special shape, classified as both 2D and 3D,
  that contains no geometric points.
  It's the identity element for the ``union`` operation.

``everything``
  A special infinite shape, classified as both 2D and 3D,
  that contains all geometric points.
  It's the identity element for the ``intersection`` operation.

2. Boolean (Set Theoretic) Operations
=====================================

``complement shape``
  Reverses inside and outside, so that all points inside the argument
  shape are outside the result shape, and vice versa.
  But the boundary doesn't change.
  If the input is a finite shape, the output will be infinite.

``union (shape1, shape2, ...)``
  Construct the set union of a list of zero or more shapes.
  
  The colours of shapes later in the list
  take precedence over shapes earlier in the list.
  This follows the metaphor of ``union`` as an additive operation
  where later shapes are "painted on top of" earlier shapes.

  ``union`` is an associative operation with ``nothing``
  as the identity element, meaning it is a monoid.
  The empty list is mapped to ``nothing``.
  If all of the shapes have the same colour, then
  ``union`` is commutative.

``intersection (shape1, shape2, ...)``
  Construct the set intersection of zero or more shapes.
  
  The colour of the first shape takes precedence.
  This is the opposite of the ``union`` convention.
  It follows the metaphor of ``intersection`` as a subtractive operation
  where the first shape is primary, and subsequent shapes indicate which parts of
  the primary shape not to remove.
  It is consistent with the traditional definition
  of ``difference(s1,s2)`` as ``intersection(s1,complement(s2))``.

  ``intersection`` is an associative operation.
  The empty list is mapped to ``everything``.
  If all of the shape arguments have the default colour,
  then ``everything`` is the identity element,
  and ``intersection`` is commutative and a monoid.
  
``difference (shape1, shape2)``
  A binary operation that subtracts shape2 from shape1,
  preserving the colour of shape1.

``symmetric_difference (shape1, shape2, ...)``
  The result contains all of the points that belong to exactly one shape in the list.
  
  This is an associative, commutative operation with ``nothing`` as its identity element.

3. Transformations
==================
A transformation is an operation that maps a shape S1 onto another shape S2,
by mapping each point (x,y,z) within S1 onto the point f(x,y,z) within S2.

Rigid Transformations
---------------------
Distance-preserving transformations of 2D and 3D shapes.
If the input has an exact distance field, the output is also exact.

``move (dx,dy) shape``
  Translate a 2D or 3D shape across the XY plane.

``move (dx,dy,dz) shape``
  Translate a 3D shape.

``rotate angle shape``
  Rotate a 2D or 3D shape around the Z axis, counterclockwise,
  by an angle measured in radians.

``rotate {angle, axis} shape``
  Rotate a 3D shape around the specified axis, counterclockwise,
  by an angle measured in radians.

``rotate quaternion shape``
  TODO

``reflect_x shape``
  Reflect a 2D/3D shape across the Y axis/YZ plane,
  mapping each point (x,y)/(x,y,z) to (-x,y)/(-x,y,z).

``reflect_y shape``
  ditto

``reflect_z shape``
  ditto

``reflect normal shape``
  TODO

``at p t shape``
  Apply a transformation ``t`` to a shape,
  treating the point ``p`` as the origin point of the transformation.
  
  Example: ``square 2 >> at (1,1) (rotate(45*deg))``
  rotates the square around the point (1,1).

``align alignspec shape``
  TODO: Using the shape's bounding box,
  translate the shape to align it relative to the origin,
  as specified by ``alignspec``.
  
  ``alignspec ::= {x: aspec, y: aspec, z: aspec}``
  
  Each field of alignspec is optional, and aspec is one of:
    
  * ``above d`` -- a point that is ``d`` above the top of the shape's bounding box.
  * ``below d`` -- a point that is ``d`` below the bottom of the shape's bounding box.
  * ``within k`` -- ``k`` is between -1 (the bottom of the bounding box)
    and +1 (the top of the bounding box). 0 is the centre.
  * ``centre`` -- centre of the shape's bounding box, same as ``within 0``.
    
  Eg, ``align {z: above 0}`` aligns the bottom of the shape with ``z==0``.
  
  See also: General Library of Relativity
  https://github.com/davidson16807/relativity.scad/wiki

``row_x d shapes``
  Move each shape in ``shapes`` along the X axis
  so they are lined up in a row, separated by gaps of distance ``d``.
  The group is centred on the origin along the X axis.

Non-Rigid Transformations
-------------------------
Non-distance-preserving transformations of 2D and 3D shapes.

``scale k shape``
  Isotropic scaling by a scale factor of ``k`` of a 2D or 3D shape.

``scale (kx, ky) shape``
  Anisotropic scaling of a 2D or 3D shape across the XY plane.

``scale (kx, ky, kz) shape``
  Anisotropic scaling of a 3D shape.

``shear_x kx shape``
  2D horizontal shear, defined on 2D and 3D shapes, mapping ``(x,y,z)`` to ``(x + kx*y, y, z)``.
  If ``kx>0``, this maps a unit square to a right-tilting parallelogram of height 1 and width ``1+kx``.
  So ``shear_x 1`` will double the width of a square, and ``shear_x 2`` will triple the width.
  
  Want to specify the shear factor as a tilt angle, like SVG skewX(a) or CSS skew(a)?
  Use ``shear_x(tan a)``.
  The tilt angle is measured clockwise from the +Y axis, ``a==0`` means no tilt.
  
  TODO: distance field is bad.
  
``shear_xy (kx,ky) shape``
  3D horizontal shear, defined on 3D shapes, mapping ``(x,y,z)`` to ``(x + kx*z, y + ky*z, z)``.
  
  TODO

``taper_x (kx0, y0, kx1, y1) shape``
  Local 2 dimensional taper along the Y axis, between y==y0 and y==y1.
  When y<=y0, x values are scaled by the factor kx0.
  When y1<=y, x values are scaled by the factor kx1.
  When y0<y<y1, x values are scaled by a factor that is a linear ramp
  between kx0 and kx1.

``taper_xy (kx0, ky0, z0, kx1, ky1, z1) shape``
  Local 3 dimensional taper along the Z axis, between z==z0 and z==z1.
  When z<=z0, x and y values are scaled by the factors kx0 and ky0.
  When z1<=z, x and y values are scaled by the factors kx1 and ky1.
  When z0<z<z1, x/y values are scaled by factors that are a linear ramp
  between kx0-kx1/ky0-ky1.

``twist d shape``
  Twist a 3D shape around the Z axis. One full revolution for each ``d`` units along the Z axis.
  Lines parallel to the Z axis will be twisted into a helix.
  
  TODO: distance field is bad.

``bend d shape``
  Take the upper half of the XY plane between X==-d/2 and X==d/2,
  and wrap that radially around the origin to cover the XY plane,
  by mapping rectangular coordinates in the source region to polar coordinates
  in the target region.
  
  TODO: bad distance field.

2D -> 3D Transformations
------------------------

``extrude d shape``
  ``extrude`` converts a 2D shape to a 3D shape,
  linearly extruding it equal distances along the positive and negative Z axis,
  with total height ``d``.
  Similar to Autocad ``extrude`` and OpenSCAD ``linear_extrude``.
 
  * ``extrude_m``: mitred distance field.
  * ``extrude_e``: exact distance field.

``revolve shape``
  The half-plane defined by ``x >= 0`` is rotated 90°, mapping the +Y axis to the +Z axis.
  Then this half-plane is rotated around the Z axis, creating a solid of revolution.
  Similar to Autocad ``revolve`` and OpenSCAD ``rotate_extrude``.

``cylinder_extrude (d, d2) shape``
  TODO:
  An infinite strip of 2D space running along the Y axis
  and bounded by ``-d/2 <= x <= d/2``
  is wrapped into an infinite cylinder of diameter ``d2``,
  running along the Z axis and extruded towards the Z axis.

``helix_extrude (...) shape``
  TODO: a 2D shape is swept along a helix. Similar to AutoCAD ``helix`` command.
  Note that if you ``twist`` a cylinder around the Z axis, the cross section is egg-shaped,
  not circular. By contrast, applying ``helix_extrude`` to a circle gives you a helix with
  a circular cross section.

``stereographic_extrude shape``
  The entire 2D plane is mapped onto the surface of the unit sphere
  using a stereographic projection,
  and extruded down to the origin.
  TODO

3D -> 2D Transformations
------------------------

``slice_xy shape``

``slice_xz shape``

``slice_yz shape``

Repetition
----------
``repeat_x d shape``

``repeat_xy d shape``

``repeat_xyz d shape``

``repeat_mirror_x shape``

``repeat_radial reps shape``

``repeat_helix ... shape``

4. Distance Field Operations
============================
These operations construct a shape from one or more distance fields.
In one or more of the shape arguments, it's the structure of the distance field
that matters, and not just the shape represented by that distance field.

Thus, if you want predictable and repeatable behaviour, you should restrict
distance field arguments to shape expressions that are documented to produce
either an exact or a mitred distance field. In other cases, where the SDF is
only documented as "approximate", the implementation is subject to change.

For all of the distance field operations, we only guarantee to compute a "good"
bounding box estimate if the distance field arguments are exact. Otherwise, the
bounding box may be "bad" (too small to contain the resulting shape),
and the user may need to fix this by calling ``set_bbox``.

* The reason is, for distance field operations, we need a lower bound on the
  ratio by which the distance field underestimates the distance to the boundary
  in order to compute a good bounding box estimate.
  For mitred distance fields in general, there is no lower bound. It's possible
  to determine lower bounds for some shape operations, but not in general.
* Other approaches: Compute this lower bound (if available), and store it in the shape,
  which is added complexity. Or, use an automatic bounding box estimator that uses distance field evaluation.
  
Level Set Operations
--------------------
The level set at ``d`` of a distance field is the set of all points whose distance value is ``d``.
This is also called an isocurve (in 2D) or isosurface (in 3D).

``offset d shape``
  Construct the shape bounded by the level set at ``d`` of the shape argument's distance field.
  
  * d > 0: inflate the shape, blow it up like a balloon.
  * d == 0: no effect.
  * d < 0: deflate the shape.
 
  If the distance field is exact, then you get the "rounded offset" of the shape.
  For positive (negative) ``d``,
  convex (concave) vertices and edges are rounded off
  as if by a circle or sphere of radius ``d``.
  [Also known as Minkowski sum (difference) of a circle or sphere of radius ``d``,
  or dilation (erosion) with a ball of radius ``d`` in Mathematical Morphology.]

  If the distance field is mitred, the result is a "mitred offset".
  Vertices and edges are preserved.

  ``offset`` can be used for debugging, to help visualize the distance field.
  
  Bounding box: If ``shape`` has an exact distance field, then we can compute a
  good bounding box, which is exact if ``shape`` has an exact bounding box
  and if ``d>=0``. If ``shape`` has a mitred or approximate distance field,
  we can only guarantee a good bounding box if ``d<=0``.

``shell d shape``
  Construct a shell of thickness ``d``,
  whose boundaries are plus or minus ``d/2``
  from the original surface or perimeter of ``shape``.

``pancake d shape``
  ``pancake`` converts a 2D shape into a 3D "pancake" of thickness d.
  The edges are rounded. The corners are rounded, if ``shape`` has an exact
  distance field, or sharp, if ``shape`` has a mitred distance field.

Morph Operations
----------------
In which we linearly interpolate between two distance fields.

``morph k (shape1, shape2)``
  Linearly interpolate between the SDFs of shape1 and shape2.
  ``k=0`` yields shape1, ``k=1`` yields shape2.

``loft d (shape1, shape2)``
  Like ``extrude``, except that you specify a lower cross section (``shape1``)
  and a upper cross section (``shape2``)
  and we linearly interpolate between the two SDFs while extruding.
  Similar to Autocad ``loft``.

  TODO: bad distance field.

Nested Distance Fields
----------------------
In which the output of one distance field is fed as input to another distance field.

``perimeter_extrude perimeter cross_section``
  A generalized torus.
  Sweep the origin point of a 2D shape (called ``cross_section``) 
  around the perimeter (distance field zero points) of another 2D shape
  (called ``perimeter``).
  The ``cross_section`` shape is offset from the perimeter based its distance
  from the origin.

  If ``perimeter`` has an exact distance field, then it's like
  a Minkowski Sum of ``cross_section`` with the zero points
  of ``perimeter``, with ``cross_section`` held orthogonal to the XY plane.

  Example: ``torus {major: d1, minor: d2} = perimeter_extrude (circle d1) (circle d2)``

  Example: ``revolve shape = perimeter_extrude (circle 0) shape``

  The behaviour when sweeping around a ``perimeter`` vertex
  depends on whether the ``perimeter`` shape has an exact or mitred distance
  field: the result is a rounded or sharp transition.
  If ``perimeter`` has an approximate distance field, then any
  deformations in that distance field will deform the ``cross_section``.

Blended Union, Intersection and Difference
------------------------------------------
A blended union is a generalized union that smoothly joins nearby objects.
The same code (which I call a "blending kernel") can also define
a blended intersection and a blended difference, which smooth away
new edges created by the intersection or difference. There are many blending
kernels.

Blending operations are sensitive to the
structure of the distance fields of their arguments.
A blended union uses the positive distance fields near the surfaces of the
shapes being blended to construct additional material to bridge the gaps
between the two shapes.

---------

The ``smooth`` blending kernel comprises:

* ``smooth_union r (shape1, shape2)``
* ``smooth_intersection r (shape1, shape2)``
* ``smooth_difference r (shape1, shape2)``

The parameter ``r`` controls the size/radius of the blending band.

``smooth_union`` is an implementation of what I call The Elliptic Blend,
since it creates a fillet with an elliptical shape. This blend is fast,
easy to use, and good enough for most purposes.

The Elliptic Blend is a popular blending operation that has been rediscovered or reinvented
many times; every author comes up with a different name and a different algorithm,
but the behaviour is the same:

* "The Potential Method for Blending Surfaces and Corners" by Hoffman and Hopcroft (1987).
  Their blend is controlled by 3 parameters: ``a`` and ``b`` control the blending radius,
  and ``λ`` controls the shape of the fillet. If you set ``a=b=r`` and ``λ=0``
  then you get The Elliptic Blend.
* A special case of the "superelliptic blend" by Rockwood & Owen (1987),
  "Blending Surfaces in Solid Modeling".
  The ellipse is generalized to a superellipse by passing an exponent as argument,
  and there are two ``r`` parameters, one for each shape being blended.
* Independently discovered by Christopher Olah (2011), called "rounded union" in ImplicitCAD.
* Faster implementation by Inigo Quilez as "opBlend", using his "polynomial smooth min" function.
* Even faster implementation by Dave Smith @ Media Molecule (2015), called "soft blend".
* Alternate implementation by MERCURY (same shape but different distance field), called "opUnionRound".

Note that ``smooth_union`` and ``smooth_intersection`` are binary operators:
they aren't associative and don't easily generalize to an arbitrary number of shapes.

Here are circles of diameter 2, combined using ``smooth_union`` with ``r`` values
1.2, 1.8, 2.4, 3.0, 3.6, 4.2, 5.0:

.. image:: images/blend.png

This looks very similar to the older "blobby objects" / "soft objects" / "`Metaballs`_" technique.
The Elliptic Blend is more general, since it works with all geometric shapes, not just circles and spheres.
But it's also less general, since it doesn't blend 3 or more shapes together
in an order-independent way.

.. _`metaballs`: https://en.wikipedia.org/wiki/Metaballs

Smooth blends can produce the artistic effect of "fillets" and "rounds" from mechanical engineering.
Here are ``smooth_union``, ``smooth_intersection`` and ``smooth_difference``
applied to a unit cube and a cylinder with ``r=.3``:

.. image:: images/smooth_blends.png

Here's the appearance of a fillet (with the same ``r``) for different
angles: 90°, 45°, 135°.

.. image:: images/fillet_angles.png

At 90°, the fillet is a quarter-circle with radius ``r``.
At other angles, the fillet deforms to an ellipse.
This might be bad for engineering, if you need a constant radius fillet,
but it's good if you are animating an organic form (like a leg attached to a torso),
and you want a constant-area fillet that looks realistic as the joint is animated.

Here's a fillet of a butt joint, same parameters as above.
To get a rounded fillet in this example, the rectangles must have exact distance fields,
so I used ``rect.exact``. This shows that the bounding box of ``smooth_union`` can be
bigger than the bounding box of ``union``. It also shows an example of a "bulge".

.. image:: images/butt_fillet.png

The "bulge" behaviour of the Elliptic Blend is considered undesirable by many people,
and there are more sophisticated blends available that avoid it.
The bulge can also be used artistically: Quilez has used it to create knee and knuckle joints
in cartoonish creatures.

As a special case, ``smooth_union r (s, s)`` is the same as ``offset (r/4) s``.
This is specific to my current code. This seems to be the worst case
for bounding box inflation, so we can use this to compute bounding boxes.

Distance field: approximate. Haven't seen a bad distance field during testing.

Bounding box: approximate.

TODO: enhance ``smooth`` blending kernel to support N-ary blends.

------

TODO: various blending kernels from MERCURY, like ``chamfer``.

TODO: investigate advanced blending primitives from

* "A Gradient-Based Implicit Blend" (2013),
  http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.592.5451&rep=rep1&type=pdf
* "Extrusion of 1D implicit profiles: Theory and first application" (2001)
  https://www.irit.fr/recherches/VORTEX/publications/rendu-geometrie/IJSM2001_Barthe_et_al.pdf

5. Shape Debugging
==================
``show_axes shape``
  Add an X/Y or X/Y/Z axis display to the shape.

``show_bbox shape``
  TODO: Visualize the bounding box, so you can check if it is bad (too small to contain the shape).

``set_bbox bbox shape``
  TODO: Manually fix a bad bounding box.

``show_dist shape``
  Visualize the signed distance field on the XY plane.
  Green channel: contour lines inside the shape (distance <= 0).
  Blue channel: contour lines outside the shape (distance > 0).
  Red channel: > 0 at points where the gradient > 1, ramping to full
  intensity where gradient >= 2.
  If distance is NaN (something that can only happen on the GPU),
  the colour is white.
  If distance is infinity, the colour is vivid cyan.
  If distance is -infinity, the colour is dark cyan.

..
  ``show_gradient (j,k) shape``
  TODO: Visualize a signed distance field by displaying gradient values.
  Gradient values < j are displayed in black.
  Gradient values > k are displayed in white.
  Gradient values between j and k are displayed using a spectrum,
  where j is red and k is violet.
..  
  You can start with (1,2) then use binary search to find the
  Lipschitz constant of a distance field, by visual inspection.

``lipschitz k shape``
  Repair a distance field whose Lipschitz constant k is != 1.
  If k < 1 then rendering via sphere tracing is slower than necessary.
  If k > 1 then rendering will fail.
  The argument ``k`` is the actual Lipschitz constant of ``shape``.
  
  If an experimental shape isn't rendering correctly,
  then ``shape >> lipschitz 2`` is often a quick way to fix the problem.
  If the distance field is not Lipschitz continuous, then ``lipschitz`` can't help you.

6. TODO: Missing/Future Shape Operations
========================================

SDF Data Structures and Rendering Algorithms
--------------------------------------------
In Curv, shapes are represented by signed distance fields.
In Curv 1.0, SDFs are represented using Function Representation (F-Rep),
and rendered on a display using Sphere Tracing.

However, I want to support a wide range of shape operations:
primitive shapes, shape operators, operations that import and export shapes
from files. In order to support the full range of shape operations,
I'll need to use some of the alternate data structure representations of SDFs,
not just F-Rep.

Mesh Import
-----------
I want the ability to import an STL file (and other mesh file types like OBJ, AMF, 3MF).
Unfortunately, meshes are probably the worst possible representation for getting geometric data into Curv.
So it won't be easy.
This is a research project, it won't be in the 1.0 release.

There are two use cases: the mesh is an exact representation of the desired shape,
or it is an approximation.

Exact Meshes
  The mesh is an exact representation of a polyhedron; it isn't an
  approximation of a curved shape.
  
  If the polyhedron has only a small number of faces, then you can
  represent it as an intersection and union of half-spaces.
  But the rendering time would be proportional to the number of half-spaces,
  so this approach doesn't scale.
  [Starting point: a Curv function that reads an STL file, returns a list of triangles.]
  
  The polyhedra that appear in math-inspired art tend to be highly symmetrical.
  The best representation of these polyhedra in Curv is as a compact CSG tree
  that explictly encodes all of the symmetries. Automatically converting a mesh to this
  representation is tricky: it would be better to get the original "source code"
  used to generate the mesh file, and port that to Curv.
  
  Alternatively, maybe we can design an efficient data structure for representing
  the distance field of a complex polyhedron?

Approximate Meshes
  The mesh is an approximation to a curved surface.
  
  Sometimes, the mesh is generated as an approximation of
  a more exact digital representation, like an OpenSCAD program, or a parametric
  spline created by a CAD program. In these cases, it would be better
  to convert the original exact representation directly to Curv, bypassing
  the intermediate mesh, since constructing a mesh throws away information
  and adds noise.
  
  In other cases, the mesh is produced by scanning a physical object,
  in which case the mesh is created from a point cloud representation (from a 3D scanner),
  or from a voxel array from a CT scanner or MRI scanner.
  [In the FastRBF paper cited below, it is stated that it's better to start with the original
  point cloud or CT scan data, since the constructed mesh has added noise (extraneous
  vertices).]

  Suppose we have a high-triangle-count approximation to a curved surface,
  like the Yoda bust on Thingiverse (614278 triangles).
  Our best strategy is to convert this into a more compact and efficient representation
  that is an approximation to the polyhedral mesh and reconstructs the curved surfaces
  while preserving edges.
  
  Converting a large mesh to a volumetric format is slow (minutes),
  so I'll use a separate conversion tool and a file format.
  
  Possible requirements:
  
  * Good quality SDF, suitable for sphere tracing.
  * Handles low quality input.
    Triangle meshes are often of poor quality:
    not 2-manifold (not watertight, self intersections);
    zero area triangles; not orientable (some normals point in the wrong direction);
    excessive detail.
  * Compact representation, since it has to fit in GPU memory.
    3D voxel arrays are simple but not compact.
  * Fast SDF evaluation.
    It's likely that Yoda will compile into a large representation.
    If all of the data is accessed each time the Yoda SDF is evaluated,
    then evaluation will be too slow. We'd prefer a compiled representation where only a small fraction
    of the data needs to be accessed when evaluating the SDF at a given point.
    Trees and arrays indexed by geometric location have the right kind of access properties.
  * GPU acceleration.
  
  This has been an active area of research for decades. There are lots of possibilities.
  
  * **3D voxel arrays** are simple and popular. Nothing is faster on a GPU.
    Each grid element contains a distance value, and the distance value at a point
    is reconstructed by interpolation using GPU texture hardware.
    
    They can take up a lot of memory, though. A 128x128x128 grid, with 16 bits per sample,
    is 4MBytes, which is tractable. Doubling the linear resolution grows the memory
    requirements by 8 times.
  
    Use GPU hardware to quickly convert a mesh to a voxel array.
    "Interactive 3D Distance Field Computation using Linear Factorization" [2006].
    http://gamma.cs.unc.edu/GVD/LINFAC/

    `Signed Distance Fields for Polygon Soup Meshes`_ (2014):
    Input is polygon soup. Triangles don't need to be correctly oriented,
    mesh doesn't need to be 2-manifold.
    The output is a voxel array.

  * An **ASDF** (Adaptively sampled Signed Distance Field) is essentially a voxel array
    that is compressed using an octree.
    "Adaptively sampled distance fields: A general representation
    of shape for computer graphics" [Susan Frisken, 2000].
    Antimony uses this representation.
    Evaluating an ASDF on a GPU (a requirement for Curv) requires novel data structures,
    which are not in the original research.
   
    GPU-Accelerated Adaptively Sampled Distance Fields (2008):
    http://hyperfun.org/FHF_Log/Bastos_GPU_ADF_SMI08.pdf
    Input is a 2-manifold mesh, output is an ASDF (adaptively sampled distance field)
    which is then rendered on a GPU using sphere tracing.
  
    Use a GPU to create and then evaluate an ASDF.
    "Exact and Adaptive Signed Distance Fields Computation
    for Rigid and Deformable Models on GPUs" [2014]
    http://graphics.ewha.ac.kr/gADF/gADF.pdf
  
    An hp-ASDF is a more sophisticated ASDF.
    "Hierarchical hp-Adaptive Signed Distance Fields" (2016)
    https://animation.rwth-aachen.de/media/papers/2016-SCA-HPSDF.pdf
  
  * **Radial Basis Functions**
    are a kind of spline representation with an associated distance field.
    
    * Any SDF can be converted to RBF form. This suggests that an expensive SDF described
      using Curv could be converted to an approximate RBF that is cheaper to evaluate.
    * If you convert the resulting RBF back to a mesh, applications include mesh simplification
      and mesh repair.
    * "Gradients and higher derivatives are determined analytically and are continuous and smooth",
      avoiding a problem with discretely sampled SDFs, which tend to be discontinuous across cell boundaries.
    
    "Reconstruction and Representation of 3D Objects with Radial Basis Functions" (2001)
    http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.58.1770&rep=rep1&type=pdf
    
    This is the FastRBF method. It is "difficult to implement".
    It has the limitation that the RBF is "global" [non compactly supported],
    meaning you have to evaluate the entire RBF
    (potentially containing a large number of "centres" or spline points) to query the SDF at any point.
    So SDF evaluation would be slow.
    
    "Implicit Surface Modeling Suitable for Inside/Outside Tests with Radial Basis Functions" (2007)
    http://vr.sdu.edu.cn/~prj/papers/p20071019.pdf
    
    Easier to implement. Uses compactly supported basis functions.
    Produces a more exact distance field (than other methods).
    
    "Modelling and Rendering Large Volume Data with Gaussian Radial Basis Functions" (2007)
    https://www.derek.juba.name/papers/RBFVolume_Tech.pdf
    
    This paper puts the RBF centres into an octree to speed up rendering (on a GPU).
    You can dynamically trade off accuracy for rendering speed by controlling how deep
    you descend the octree.
  
A Hybrid Geometry Engine
   We could abandon the idea of converting a mesh to an SDF.
   Instead, implement a hybrid geometry engine, where some shapes are represented
   as meshes, some are represented as SDFs, and some are hybrid unions of
   meshes and SDFs. Some operations work on all 3 representations (eg,
   affine transformations). Some operations work only on meshes, or only on SDFs.
   You can convert an SDF to a mesh (but not vice versa).
   A top level scene is a union of meshes and SDFs, rendered using some hybrid
   Z-buffer algorithm. But, there are a lot of Curv operations that won't work
   on Yoda, and the whole implementation is twice as complex.

.. _`Signed Distance Fields for Polygon Soup Meshes`: http://run.usc.edu/signedDistanceField/XuBarbicSignedDistanceField2014.pdf

I'll begin by using voxel arrays, since they are the industry standard.
I'll use a separate tool (eg, https://github.com/matejd/DistanceFieldGen) to convert a mesh
to a (compressed) 3D texture SDF, stored in a KTX file. I'll extend Curv to import these KTX files.

Voxel Arrays
------------
A voxel array containing signed distance values is an alternate representation of an SDF.
Interpolation (using GPU texture hardware) is used to compute the distance value at a point.
All of the Curv shape operations will work on this representation.

Benefits:

* Uniformly fast evaluation on a GPU.
* An F-Rep SDF that is too expensive to evaluate during interactive previewing
  can be sped up by conversion to a voxel array.
* Easy and fast to convert a mesh file to a voxel array.
* There are useful shape operators that require a voxel array, not an F-Rep SDF.
  Eg, "Level Set Surface Editing Operators", http://www.museth.org/Ken/Publications_files/Museth-etal_SIG02.pdf
  Or, possibly, some of the ShapeJS operations: http://shapejs.shapeways.com/

The disadvantage is that it is an approximate sampled representation, not an
exact representation. And storage requirements increase with the cube of the resolution.

Convex Hull
-----------
An OpenSCAD operation that is difficult/expensive to implement in F-Rep.
It's a powerful and intuitive operation, so it would be nice to have for that reason alone.

Convex Hull is used to create a skin over elements that form the skeleton of the desired shape.
There are probably better and cheaper ways to accomplish this in F-Rep,
so this operation is not a must-have.

Convex Hull could be implemented in restricted form as a Polytope Operator (see below).
This means it's not supported on curved surfaces.

Convex hull of two copies of the same shape is equivalent to sweeping that shape
over a line segment: there is a separate "TODO" entry for Linear Sweep.

Minkowski Sum
-------------
An OpenSCAD operation that is difficult/expensive to implement in F-Rep.
I personally like Minkowski sum, but there is a learning curve in understanding
how it works. It's not intuitive to people who first encounter it.

The most common Minkowski sum idioms have cheaper direct implementations in F-Rep
which are also easier to understand.

* Rounded offset at distance d: Minkowski sum with a sphere of radius d, or ``offset d``
  of a shape with an exact distance field.
* Shell: in Curv, ``shell``.
* Morph between two shapes: in Curv, ``morph``.
* Sweep a 3D solid along a 3D curve: This has its own entry in the TODO list,
  and might be easier than a general Minkowski sum implementation.

My intuition says that Minkowski sum ought to be implementable as a Nested Distance Field
operation on shapes with exact distance fields, analogous to ``perimeter_extrude``.
But it's not quite as simple as that, and an actual implementation is likely to be expensive.

Splines
-------
Spline support is important for compatibility with external tools that create spline curves and surfaces.
Adobe Illustrator supports cubic Bezier curves only. The SVG file format supports quadradic and cubic Beziers.
Inkscape can read quadratic Beziers, but it elevates them to cubic for editing.
3D CAD programs (FreeCAD, Rhino, etc) additionally support B-Splines and NURBS (and sometimes T-Splines).

* Sweep a spline curve using a circle/sphere in 2D/3D. Open or closed curve.
  A solution for cubic Bezier curves is outlined in `Sphere Tracing`_, based on code from Graphics Gems:
  https://github.com/erich666/GraphicsGems/blob/master/gems/NearestPoint.c.
  
  * Given a point ``p`` and 4 Bezier control points, construct a 5th order Bezier equation
    whose solution finds the point on the curve closest to ``p``.
  * Find the roots of the 5th degree equation using iterative root finding.
    The roots are parameter values ``t`` for the Bezier curve.
  * Evaluate the Bezier at each root ``t`` to produce a set of candidate points.
    Extend the set of candidate points with the first and last control point, which are
    the endpoints of the curve. Select the candidate point that is closest to ``p``.
  * The distance from ``p`` to the candidate point gives an exact SDF for a zero-width Bezier curve.
    Subtract ``d`` from the SDF to sweep the curve with a ball of radius ``d``.

* Construct a shape by filling the space bounded by a closed spline curve (2D)
  or surface (3D).

Mathematica has BezierCurve, BSplineCurve, and BSplineSurface (for NURBS).

Circle/Sphere Sweep of a Parametric Curve
-----------------------------------------
Spline curves are a special case of parametric curves.
There are lots of interesting mathematical art objects defined by parametric equations.
Eg, I'd like to sweep out a `trefoil knot`_ with a sphere,
using the parametric equations::

  x = sin t + 2*sin(2*t)
  y = cos t - 2*cos(2*t)
  z = -sin(3*t)

.. _`trefoil knot`: https://en.wikipedia.org/wiki/Trefoil_knot

This would be trivial if we could analytically convert these parametric equations to implicit form.
I'm not sure there is a general solution to this problem.
According to `Geometric and Solid Modeling`_, chapter 5:

  General techniques exist for converting [algebraic surfaces and curves]
  from parametric to implicit form, at least in principle,
  and we review here a simple version based on the Sylvester resultant. In
  Chapter 7, we show how to use Grobner bases techniques for this purpose.

Lots of useful curves aren't algebraic (ie, polynomial), like the helix,
the sine wave, and the trefoil knot. (Is there a more general solution for
analytic conversion?)

I also know that an analytic solution can be too expensive to use.
In `The Implicitization of a Trefoil Knot`_, Michael Trott
converts the trefoil knot parametric equation to implicit form, using Mathematica.
"The result is a large polynomial.
It is of total degree 24, has 1212 terms and coefficients with up to 23 digits."

.. _`The Implicitization of a Trefoil Knot`: https://www.google.ca/url?sa=t&rct=j&q=&esrc=s&source=web&cd=13&ved=0ahUKEwj9o7-S9tvUAhWl24MKHYjLCwAQFghPMAw&url=http%3A%2F%2Fwww.mathematicaguidebooks.org%2Fscripts%2Fdownload_file.cgi%3Fsoftware_download%3DSample_Section_Symbolics.nb.pdf&usg=AFQjCNHYR408D7qpaYvJC7500ylz9iY0Mw

What about a numerical solution?
According to "Image Swept Volumes" (Winter and Chen),
accurate numerical solutions can often be quite expensive.
(Fine, but let's try it anyway. How do I do that?)

So we are looking for some way to remove the heavy lifting from the trefoil knot SDF distance function.

For example, compile the parametric equations into a data structure that can be efficiently queried
by the distance function to produce a reasonable approximation of the curve.
Sample the parametric curve, either at regular intervals, or adaptively (higher sampling
rate where the curvature is higher). Put the sample values into a balanced space partitioning
tree structure. The distance function looks up the nearest sampled points in the tree
and then:

1. uses polynomial interpolation to estimate the nearest point on the curve.
2. uses root finding to find the value of t for the closest point on the curve.

Either way, we are creating an approximation to the curve, within some error tolerance.
If a non-linear transformation is applied, and part of the curve is scaled to a larger size,
then a smaller error tolerance may be required in the scaled region of the curve.
So let's think about how to dynamically
determine the appropriate error tolerance during SDF evaluation time.

Precompiling the parametric equations to a data structure won't work if the equations
contain coefficients derived from SDF evaluation time data (x,y,z,t coordinates).

.. _`Geometric and Solid Modeling`: https://www.cs.purdue.edu/homes/cmh/distribution/books/geo.html

Linear Sweep
------------
Sweep an arbitrary 2D/3D shape along a 2D/3D line segment.

General Sweep
-------------
Sweep an arbitrary 2D/3D shape along an arbitrary 2D/3D curve.

General Extrude
---------------
Sweep an arbitrary 2D shape along an arbitrary 3D curve.
The shape is normal to the curve at all points.
A generalization of ``extrude``.

research:

* "Image Swept Volumes", Winter and Chen, http://vg.swan.ac.uk/vlib/DOWNLOADS/ISV.pdf

Pixelate
--------
Transform a 2D shape so that it appears to be made of uniformly sized and coloured pixels,
or transform a 3D shape to voxels. The goal is to create a common
artistic effect: eg, make a shape look like it was modeled in Minecraft.

Convolution
-----------
A low pass filter would remove high frequency components from a shape,
rounding off sharp vertices and edges, and in effect "blurring" the shape.
Mathematical convolution is a way to implement this.

Local Deformations
------------------
These operations treat a shape as a lump of clay,
in which local regions can be arbitrarily deformed
while leaving the rest of the shape unmodified.
They are found in `digital sculpting`_ programs like ZBrush.

.. _`digital sculpting`: https://en.wikipedia.org/wiki/Digital_sculpting

CorelDraw has Smear, Twirl, Attract and Repel operators,
which perform smooth local translations, rotations and +/- scaling.
This seems like a good starting point.
Antimony has Attract and Repel in open source.

Drawing Text using a Font
-------------------------
Signed distance fields are now considered the best way to render text using a GPU.
For example, the Qt graphics toolkit uses SDFs for text rendering.
This fits into Curv really well.

The trick is to convert each character into a discretely sampled SDF, stored in a texture.
This happens before SDF evaluation time (rendering).
During rendering, we do interpolated texture lookups to get the value of a character SDF at a point.

Conway Polyhedron Operators
---------------------------
Implement the `Conway polyhedron operators`_.
Existing polyhedron constructors like ``cube``, ``icosahedron``, etc, are modified so that they
can be used as input values.

``antiprism n``
  3d shape constructor, one of the Conway primitives.

* In OpenSCAD, by Kit Wallace: https://github.com/KitWallace/openscad/blob/master/conway.scad
* In JavaScript/VRML, by George Hart: http://www.georgehart.com/virtual-polyhedra/conway_notation.html
* HTML5: http://levskaya.github.io/polyhedronisme/

.. _`Conway polyhedron operators`: https://en.wikipedia.org/wiki/Conway_polyhedron_notation

Polytope Operators
------------------
A polytope is either a polygon or a polyhedron.
Polytopes contain additional shape attributes representing the vertices and faces.
Polytope operators are operations that only make sense on polytopes, not on general curved shapes.
They operate directly on the vertices and faces.

* The Conway polyhedron operators are an example, although some of these operators
  may not work on general polyhedra (to be investigated).
* Convex hull is possibly another example. It's a standard operation on polyhedral meshes,
  but I don't have an implementation for SDFs.
* The boolean operators and affine transformations take arbitrary shapes as arguments (including polytopes)
  but do not return polytopes as results. We could generalize these operators to return polytopes, when given
  polytopes as input. Note that ``union`` is very cheap, and ``polytope_union`` is very expensive, and also
  numerically unstable (fails for some valid inputs).
* ``polygonize`` maps an arbitrary shape to a polytope that approximates the shape.

Supershapes
-----------
Superquadrics were popularized by Alan Barr as a solid modelling primitive.
Includes superellipsoids, superhyperboloids, supertoroids.
See `Sphere Tracing`_ for distance functions.

Supershapes, constructed using the Superformula, are a generalization of Superquadrics.
Implicit function representation for supershapes: http://le2i.cnrs.fr/IMG/publications/PG05.pdf

Hypertextures
-------------
Using Perlin noise (fractal noise) to deform a shape.
See `Sphere Tracing`_.

Fractals
--------
Mandelbulber uses SDFs and sphere tracing to render fractals
constructed using a variety of algorithms.
These algorithms could be packaged as Curv shape constructors.

http://mandelbulber.com/

Voronoi Diagrams
----------------
In 2 and 3 dimensions, are a popular modeling technique in 3D printed geometric art.

Hyperbolic Geometry
-------------------
The math behind Escher's Circle Limit prints.

Carole Blanc
------------
* http://dept-info.labri.fr/~blanc/abst.html

A number of these papers are directed towards the creation of a Curv-like
geometry system.

Spirals
-------
``log_spiral ...``
  TODO: logarithmic spiral

``linear_spiral ...``
  TODO: linear (aka Archimedean) spiral

``repeat_spiral ... shape``

7. Bibliography
===============
* John C. Hart, `Sphere Tracing`_
* Inigo Quilez, `Modelling with Distance Functions`_
* MERCURY, `hg_sdf`_: A glsl library for building signed distance functions
* Christopher Olah, `Manipulation of Implicit Functions With an Eye on CAD`_

.. _`Sphere Tracing`: http://graphics.cs.illinois.edu/sites/default/files/zeno.pdf
.. _`Modelling with Distance Functions`: http://iquilezles.org/www/articles/distfunctions/distfunctions.htm
.. _`hg_sdf`: http://mercury.sexy/hg_sdf/
.. _`Manipulation of Implicit Functions With an Eye on CAD`: https://christopherolah.wordpress.com/2011/11/06/manipulation-of-implicit-functions-with-an-eye-on-cad/
