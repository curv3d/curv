Shape Constructors
==================

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
  * ``square.mitred d``: mitred distance field, simple code, cheap to compute.
  * ``square.exact d``: exact distance field, more expensive.

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
  * ``rect_m ...``: mitred distance field, cheaper to compute.
  * ``rect_e ...``: exact distance field, more expensive.

``regular_polygon n``
  Construct a regular polygon, centred on the origin,
  with ``n`` sides, whose bottom edge is parallel to the X axis.
  Cost: constant time and space, regardless of ``n``.
 
  * ``regular_polygon n``: A prototypical instance,
    inscribed by the unit circle.
  * ``regular_polygon(n).circumratio``: Ratio of circumradius over inradius.
  * ``regular_polygon n d``: Construct a regular polygon
    whose inscribed circle has diameter ``d``.
  * ``regular_polygon_m n d``: mitred distance field.
  * ``regular_polygon_e n d``: exact distance field (TODO).

  TODO: Calls to regular_polygon should compile into optimized code,
  like http://thndl.com/square-shaped-shaders.html

..
  Example: ``regular_polygon 5``

..
  |pentagon|

.. |pentagon| image:: ../images/pentagon.png

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
  
  * ``ellipsoid.exact``: exact distance field, more expensive to compute (TODO).

``cylinder {d, h}``
  Construct a cylinder, centered on the origin, whose axis of rotation is the Z axis.
  Diameter is ``d`` and height is ``h``.
 
  * ``cylinder.mitred``: mitred distance field.
  * ``cylinder.exact``: exact distance field, more expensive.

``cone {d, h}``
  Construct a cone.
  The base (of diameter ``d``) is embedded in the XY plane and centred on the origin.
  The apex is above the origin at height ``h``.
 
  * ``cone.mitred``: mitred distance field.
  * ``cone.exact``: exact distance field, more expensive.

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

  * ``S.mitred d`` constructs an instance of S with a mitred distance field.
  * ``S.exact d`` constructs an instance of S with an exact distance field.

  TODO:

  * ``tetrahedron.exact``
  * ``octahedron.exact``
  * ``dodecahedron.exact``
  * ``icosahedron.exact``

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
