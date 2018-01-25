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
