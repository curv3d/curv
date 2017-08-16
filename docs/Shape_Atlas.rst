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
in a Colour Atlas document.)

Signed Distance Fields
----------------------
Shapes are represented internally as Signed Distance Fields (SDFs), see `<Theory.rst>`_.
This is not a unique representation: a given shape can be represented by many different SDFs.
In most cases, the user just wants to let the implementation choose an SDF that is fast
to compute and fast to render.

However, there are some Distance Field Shape Operations (see section 3)
that construct a shape based on the SDF of an input shape.
These operations are too useful to leave out.
For this reason, we document the class of SDF created by each shape operation.

There are 3 SDF classes:

exact:
  The distance field contains the exact Euclidean distance to the nearest boundary.
  The ``inflate`` operation will create a rounded offset.
mitred:
  Vertex and edge information is preserved in all isosurfaces.
  The ``inflate`` operation will create a mitred offset.
approximate:
  The SDF is implementation dependent, and may change between releases
  as the code is optimized.

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
``circle d``
  Construct a circle of diameter ``d``, centred on the origin.
  Exact distance field.

``ellipse (dx, dy)``
  Construct an axis-aligned ellipse, centred on the origin,
  with width ``dx`` and height ``dy``.
  Approximate distance field.
  
  * ``ellipse_e``: exact distance field, much more expensive to compute (TODO).

``square d``
  Construct an axis-aligned square of width ``d``, centred on the origin.
  
  * ``square_m``: mitred distance field, simple code, cheap to compute.
  * ``square_e``: exact distance field, more expensive.

``rect (dx, dy)``
  Construct an axis-aligned rectangle of width ``dx`` and height ``dy``,
  centred on the origin.
  
  * ``rect_m``: mitred distance field, simple code, cheap to compute.
  * ``rect_e``: exact distance field, more expensive.

``rect_at ((xmin,ymin), (xmax,ymax))``
  Construct an axis-aligned rectangle
  whose lower-left corner is ``(xmin,ymin)``
  and whose upper-right corner is ``(xmax,ymax)``.
  Unlike ``rect``, this function lets you construct
  half-infinite rectangles where, eg, ``ymin`` is
  finite but ``ymax`` is ``inf``.
  
  * ``rect_at_m``: mitred distance field
  * ``rect_at_e``: exact distance field (TODO)

``regular_polygon (n, d)``
  Construct a regular polygon, centred on the origin,
  with ``n`` sides, whose inscribed circle has diameter ``d``.
  Bottom edge is parallel to X axis.
  Cost: constant time and space, regardless of ``n``.
 
  * ``regular_polygon_m``: mitred distance field.
  * ``regular_polygon_e``: exact distance field (TODO).

..
  Example: ``regular_polygon(5,1)``

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

``stroke (d, p1, p2)``
  A line of thickness ``d`` drawn from ``p1`` to ``p2``,
  with semicircle end caps of radius ``d/2``.
  Exact distance field.

``half_plane_dn (d, n)``
  A half plane with normal vector ``n``,
  whose edge is distance ``d`` from the origin.
  ``n`` must be a unit vector.
  If d >= 0, the half-plane contains the origin.
  Exact distance field.

``half_plane_pn (p, n)``
  A half plane with normal vector ``n``,
  whose edge passes through point ``p``.
  ``n`` must be a unit vector.
  Exact distance field.

``half_plane_p2 (p1, p2)``
  A half-plane whose edge passes through points p1 and p2.
  Exact distance field.

``spline ???``
  TODO.
  
  * draw an open spline curve, by sweeping a circle along the curve.
  * draw a closed spline curve by filling the area it encloses.

``text font string ???``
  Draw text. TODO

3D Shapes
---------
``sphere d``
  Construct a circle of diameter ``d``, centred on the origin.
  Exact distance field.

``ellipsoid (dx, dy, dz)``
  Construct an axis-aligned ellipsoid, centred on the origin,
  with width ``dx``, depth ``dy`` and height ``dz``.
  Approximate distance field.
  
  * ``ellipsoid_e``: exact distance field, much more expensive to compute (TODO).

``cylinder (d, h)``
  Construct a cylinder, centered on the origin, whose axis of rotation is the Z axis.
  Diameter is ``d`` and height is ``h``.
 
  * ``cylinder_m``: mitred distance field.
  * ``cylinder_e``: exact distance field, more expensive.

``cone (d, h)``
  Construct a cone.
  The base (of diameter ``d``) is embedded in the XY plane and centred on the origin.
  The apex is above the origin at height ``h``.
 
  * ``cone_m``: mitred distance field. (TODO)
  * ``cone_e``: exact distance field, more expensive.

``torus (d1, d2)``
  Construct a torus, centred on the origin, axis of rotation is Z axis.
  Major diameter is ``d1`` (center of tube to centre of tube, crossing the origin).
  Minor diameter is ``d2`` (diameter of the tube).
  Total width of shape is ``d1+d2``.
  Exact distance field.

``box (dx, dy, dz)``
  Construct an axis-aligned cuboid of width ``dx``, depth ``dy`` and height ``dz``,
  centred on the origin.
 
  * ``box_m``: mitred distance field.
  * ``box_e``: exact distance field, more expensive.

``box_at ((xmin,ymin,zmin), (xmax,ymax,zmax))``
  Construct an axis-aligned cuboid.
 
  * ``box_at_m``: mitred distance field.
  * ``box_at_e``: exact distance field, more expensive. (TODO)

``prism (n, d, h)``
  Construct a regular right prism, centred on the origin, of height ``h``.
  The base is a regular polyhedron with ``n`` sides, whose inscribed circle has diameter ``d``,
  parallel to the XY plane.
 
  * ``prism_m``: mitred distance field.
  * ``prism_e``: exact distance field, more expensive. (TODO)

``pyramid (n, d, h)``
  Construct a regular right pyramid.
  The base is a regular polyhedron with ``n`` sides, whose inscribed circle has diameter ``d``.
  The base is embedded in the XY plane and centred on the origin.
  The apex is above the origin at height ``h``.
  TODO

``tetrahedron d``
  Construct a regular tetrahedron, centred on the origin.
  Diameter of inscribed sphere is ``d``.
 
  * ``tetrahedron_m``: mitred distance field.
  * ``tetrahedron_e``: exact distance field, more expensive. (TODO)

``cube d``
  Construct an axis aligned cube (regular hexahedron), centred on the origin.
  Diameter of inscribed sphere (aka height of cube) is ``d``.
 
  * ``cube_m``: mitred distance field.
  * ``cube_e``: exact distance field, more expensive.

``octahedron d``
  Construct a regular octahedron, centred on the origin.
  Diameter of inscribed sphere is ``d``.
 
  * ``octahedron_m``: mitred distance field.
  * ``octahedron_e``: exact distance field, more expensive. (TODO)

``dodecahedron d``
  Construct a regular dodecahedron, centred on the origin.
  Diameter of inscribed sphere is ``d``.
 
  * ``dodecahedron_m``: mitred distance field.
  * ``dodecahedron_e``: exact distance field, more expensive. (TODO)

``icosahedron d``
  Construct a regular icosahedron, centred on the origin.
  Diameter of inscribed sphere is ``d``.
 
  * ``icosahedron_m``: mitred distance field.
  * ``icosahedron_e``: exact distance field, more expensive. (TODO)

``capsule (d, p1, p2)``
  A cylinder of diameter ``d`` whose central axis extends from ``p1`` to ``p2``,
  with the addition of hemispherical end caps of radius ``d/2``.
  Exact distance field.

``half_space (d, n)``
  A half-space with normal vector ``n``,
  whose face is distance ``d`` from the origin.
  Exact distance field.
  
``half_space (p1, p2, p3)``
  A half-space whose face passes through points p1, p2, p3, which are not colinear.
  The normal vector is obtained from the points via the right-hand rule.
  Exact distance field.
  TODO

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

2. Shape Combinators
====================

Boolean (Set Theoretic) Operations
----------------------------------
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
  If all of the shapes have the default colour,
  then ``everything`` is the identity element,
  and ``intersection`` is commutative and a monoid.
  
``difference (shape1, shape2)``
  A binary operation that subtracts shape2 from shape1,
  preserving the colour of shape1.

``symmetric_difference (shape1, shape2, ...)``
  The result contains all of the points that belong to exactly one shape in the list.
  
  This is an associative, commutative operation with ``nothing`` as its identity element.

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

``rotate (angle, axis) shape``
  Rotate a 3D shape around the specified axis, counterclockwise,
  by an angle measured in radians.

``rotate quaternion shape``
  TODO

``reflect_x shape``
  Reflect a 2D/3D shape across the Y axis/YZ plane,
  mapping each point (x,y)/(x,y,z) to (-x,y)/(-x,y,z).

``reflect normal shape``
  TODO

``at p t shape``
  Apply a transformation ``t`` to a shape,
  treating the point ``p`` as the origin point of the transformation.
  
  Example: ``square 2 >> at (1,1) (rotate(45*deg))``
  rotates the square around the point (1,1).

``align alignspec shape``
  Using the shape's bounding box,
  translate the shape to align it relative to the origin,
  as specified by ``alignspec``.
  TODO

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

``taper_x k shape``
  2D dimensional taper along the Y axis.
  The x coordinate of each point in the shape is scaled based on the y height, and is mapped to ``x * (1 - y*k)``.
  At ``y==0`` there is no x scaling.
  
  Taper factor ``k==0`` means no tapering.
  For positive k, the width of the shape decreases with increasing Y height,
  reducing to zero (the vanishing point) at height ``y==1/k``.

  TODO: the distance field is bad.
  At present, the DF becomes infinitely bad in a region radiating out of the vanishing point,
  and sphere tracing fails if a ray goes through this region of chaos.
  
  The taper transformation maps an infinite number of points onto the vanishing point,
  which is a singularity. Fixing the distance field will be easier if we don't support shapes
  that contain the vanishing point within their bounding box.

generalized taper ...
  TODO

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

``loft d shape1 shape2``
  Similar to Autocad ``loft``.
  TODO

``revolve shape``
  The half-plane defined by ``x >= 0`` is rotated 90°, mapping the +Y axis to the +Z axis.
  Then this half-plane is rotated around the Z axis, creating a solid of revolution.
  Similar to Autocad ``revolve`` and OpenSCAD ``rotate_extrude``.

``cylinder_extrude (d, d2) shape``
  An infinite strip of 2D space running along the Y axis
  and bounded by ``-d/2 <= x <= d/2``
  is wrapped into an infinite cylinder of diameter ``d2``,
  running along the Z axis and extruded towards the Z axis.
  TODO

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

Morph
-----
``morph (k, shape1, shape2)``
  Linearly interpolate between shape1 and shape2.
  ``k=0`` yields shape1, ``k=1`` yields shape2.

3. Distance Field Shape Operations
==================================
These operations construct a shape from one or more distance fields.
In one or more of the shape arguments, it's the structure of the distance field
that matters, and not just the shape represented by that distance field.

``inflate d shape``
  Construct the shape bounded by the isosurface at ``d`` of the shape argument's distance field.
  
  * d > 0: inflate the shape, blow it up like a balloon.
  * d == 0: no effect.
  * d < 0: deflate the shape.
 
  If the distance field is exact, then positive d yields
  the Minkowski sum of a sphere of radius d, aka rounded offset (CAD),
  aka dilation (Mathematical Morphology).
 
  If the distance field is mitred, the result is a mitred offset (CAD).

``shell d shape``
  Hollow out the shape, replace it by a shell of thickness ``d`` that is centred on the shape boundary.

2D -> 3D Transformations
------------------------
``pancake d shape``
  ``pancake`` converts a 2D shape into a 3D "pancake" of thickness d.
  The edges are rounded. The corners are rounded, if ``shape`` has an exact
  distance field, or sharp, if ``shape`` has a mitred distance field.

``perimeter_extrude perimeter cross_section``

Blends
------
``smooth_union k (shape1, shape2)``

``smooth_intersection k (shape1, shape2)``

``smooth_difference k (shape1, shape2)``

4. Distance Field Debugging
===========================

5. Bibliography
===============
