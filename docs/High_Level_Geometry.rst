=============================
High Level Geometry Interface
=============================

Curv has a high level, easy to use interface for constructing shapes.
There is a set of standard shape constructors,
plus functions for transforming and combining shapes to make new shapes.

Every shape is marked as being 2-dimensional, 3-dimensional, or both.
2D shapes are embedded in the XY plane.
(The only standard shapes that are both are ``everything`` and ``nothing``.)

A shape can be infinite. Many shape constructors accept ``inf`` as a dimension argument.

A 2D shape with no area, or a 3D shape with no volume, is called degenerate.
Examples are geometric points, line segments or curves, and in 3D, surfaces with 0 thickness.

Points and curves are invisible in the preview window.
Infinite and degenerate shapes are useful as intermediates for constructing
shapes, even though you can't 3D print them or export them to some file formats.

Shapes have volumetric colour.
A function assigns a colour to each point in the interior and on the boundary
of every 2D and 3D shape. Primitive shapes are assigned a default yellowish colour,
which you change using the ``colour`` function.

TODO: This API is *under construction*.
I'm updating the design and updating this document in parallel.

2D Shapes
=========
``circle d``
  Construct a circle of diameter ``d``, centred on the origin.

``ellipse (dx, dy)``
  Construct an axis-aligned ellipse, centred on the origin,
  with width ``dx`` and height ``dy``.
  TODO

``square d``
  Construct an axis-aligned square of width ``d``, centred on the origin.

``rect (dx, dy)``
  Construct an axis-aligned rectangle of width ``dx`` and height ``dy``,
  centred on the origin.

``regular_polygon (n, d)``
  Construct a regular polygon, centred on the origin,
  with ``n`` sides, whose inscribed circle has diameter ``d``.
  Cost: constant time and space, regardless of ``n``.

..
  Example: ``regular_polygon(5,1)``

..
  |pentagon|

.. |pentagon| image:: images/pentagon.png

``convex_polygon vertices``
  The vertex list is in counter-clockwise order.
  TODO

``polygon vertices``
  TODO

``half_plane (d, n)``
  A half plane with normal vector ``n``,
  whose edge is distance ``d`` from the origin.
  
``half_plane (p1, p2)``
  A half-plane whose edge passes through points p1 and p2.
  TODO

``text ???``
  Draw text. TODO

3D Shapes
=========
``sphere d``
  Construct a circle of diameter ``d``, centred on the origin.

``ellipsoid (dx, dy, dz)``
  Construct an axis-aligned ellipsoid, centred on the origin,
  with width ``dx``, depth ``dy`` and height ``dz``.
  TODO

``cylinder (d, h)``
  Construct a cylinder, centered on the origin, whose axis of rotation is the Z axis.
  Diameter is ``d`` and height is ``h``.

``cone (d, h)``
  Construct a cone.
  The base (of diameter ``d``) is centered on the origin.
  The apex points up, is above the origin at height ``h``.
  Axis of rotation is the Z axis.

``torus (d1, d2)``
  Construct a torus, centred on the origin, axis of rotation is Z axis.
  Major diameter is ``d1`` (center of tube to centre of tube, crossing the origin).
  Minor diameter is ``d2`` (diameter of the tube).
  Total width of shape is ``d1+d2``.

``box (dx, dy, dz)``
  Construct an axis-aligned cuboid of width ``dx``, depth ``dy`` and height ``dz``,
  centred on the origin.

``prism (n, d, h)``
  Construct a regular right prism, centred on the origin, of height ``h``.
  The base is a regular polyhedron with ``n`` sides, whose inscribed circle has diameter ``d``,
  parallel to the XY plane.

``pyramid (n, d, h)``
  Construct a regular right pyramid.
  The base is a regular polyhedron with ``n`` sides, whose inscribed circle has diameter ``d``.
  The base is embedded in the XY plane and centred on the origin.
  The apex is above the origin at height ``h``.
  TODO

``tetrahedron d``
  Construct a regular tetrahedron, centred on the origin.
  Diameter of inscribed sphere is ``d``.

``cube d``
  Construct an axis aligned cube (regular hexahedron), centred on the origin.
  Diameter of inscribed sphere (aka height of cube) is ``d``.

``octahedron d``
  Construct a regular octahedron, centred on the origin.
  Diameter of inscribed sphere is ``d``.

``dodecahedron d``
  Construct a regular dodecahedron, centred on the origin.
  Diameter of inscribed sphere is ``d``.

``icosahedron d``
  Construct a regular icosahedron, centred on the origin.
  Diameter of inscribed sphere is ``d``.

``half_space (d, n)``
  A half-space with normal vector ``n``,
  whose face is distance ``d`` from the origin.
  
``half_space (p1, p2, p3)``
  A half-space whose face passes through points p1, p2, p3, which are not colinear.
  The normal vector is obtained from the points via the right-hand rule.
  TODO

2D -> 3D Transformations
========================

``extrude h shape``

``pancake d shape``

``loft h shape1 shape2``
  TODO

``rotate_extrude shape``
  The half-plane defined by ``x >= 0`` is rotated 90°, mapping the +Y axis to the +Z axis.
  Then this half-plane is rotated around the Z axis, creating a solid of revolution.

``cylinder_extrude (d, d2) shape``
  An infinite strip of 2D space running along the Y axis
  and bounded by ``-d/2 <= x <= d/2``
  is wrapped into an infinite cylinder of diameter ``d2``, running along the Z axis
  and extruded towards the Z axis.
  TODO

``stereographic_extrude shape``
  The entire 2D plane is mapped onto the surface of the unit sphere
  using a stereographic projection,
  and extruded down to the origin.
  TODO

Advanced CSG Operations
=======================
These are expert level CSG operations that break the abstraction of a simple world of geometric shapes,
and expose the underlying representation of shapes as Signed Distance Fields.

``perimeter_extrude perimeter cross_section``
