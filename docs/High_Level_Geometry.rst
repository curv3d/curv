=============================
High Level Geometry Interface
=============================

Curv has a high level, easy to use interface for constructing shapes.
There is a set of standard shape constructors,
plus functions for transforming and combining shapes to make new shapes.

Every shape is marked as being 2-dimensional, 3-dimensional, or both.
(The only standard shapes that are both are ``everything`` and ``nothing``.)

A shape can be infinite. Many shape constructors accept ``inf`` as a dimension argument.

A 2D shape with no area, or a 2D shape with no volume, is called degenerate.
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
