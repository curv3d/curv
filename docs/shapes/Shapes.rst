======
Shapes
======

A shape value represents a geometric shape.

For example, ...

Shape Properties
================

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
