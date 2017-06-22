What is Curv?
=============
|twistor| |shreks_donut|

.. |twistor| image:: images/torus.png
.. |shreks_donut| image:: images/shreks_donut.png

Curv is an open source 3D solid modelling language, oriented towards 3D printing, procedurally generated art, and mathematical visualization.

It's easy to use for beginners. It's incredibly powerful for experts.
And it's fast: all rendering is performed by the GPU.

Constructive Solid Geometry
===========================
Curv supports Constructive Solid Geometry (CSG)
as a high level, easy to use, and expressive API for specifying geometric shapes.
Shapes are first class values,
and are constructed by transforming and combining simpler shapes using a rich set of geometric operations.

Primitive shapes include half_space, cube, sphere, cylinder, and torus.
Unary operations, which transform one shape into another,
include translate, rotate, scale, extrude, and twist.
Binary operations, which combine two shapes to create a third shape,
include union, intersection, difference, perimeter_extrude, and morph.

Function Representation
=======================

Pure Functional Programming
===========================
Curv is a pure functional language. Why?

* simple, terse, pleasant programming style
* simple semantics
* can easily be translated into highly parallel GPU code

In Curv, geometric shapes are first class values, and are constructed by transforming and combining simpler shapes using a rich set of geometric operations. This style of specification is called CSG: Constructive Solid Geometry. It's easy and pleasant to use, very expressive, and is a good match with functional programming.

F-Rep, not Meshes
=================
Instead of triangular meshes (like OpenSCAD), Curv represents shapes as pure functions (Function Representation or F-Rep). Why?

0. F-Rep is a more powerful and expressive representation than meshes.
   Shapes can be infinitely detailed, infinitely large. Any shape that can be
   described using mathematics can be represented exactly.

1. Meshes are approximations, F-Rep is exact. As you apply a chain of successive geometry operations to a mesh,
   approximation errors can pile up.

2. With a mesh, simulating a curved surface with high fidelity requires lots of triangles (and memory).
   There is a tradeoff between accuracy of representation and memory/processing costs.
   F-Rep can represent curved surfaces exactly, at low cost.

3. The cost of mesh operations goes up, often non-linearly, with the number of triangles.
   For example, this is true for union and intersection.
   F-Rep can implement most common geometric operations, like union and intersection, in small constant time and space.

4. With a mesh, complex shapes with a lot of fine detail require lots of triangles and are very expensive.
   Examples are fractals, digital fabrics, metamaterials. OpenSCAD encounters these limits quite early.
   Many complex models that are 3D printable are out of reach.
   F-Rep can represent infinite complexity for free.

5. Unlike subtractive manufacturing (eg, CNC milling), or moulding, where you only control the boundary of an object,
   3D printing is an inherently *volumetric* manufacturing technology. 3D printers directly control the material placed at
   each voxel in a 3D volume. There is a slogan for this: In 3D printing, complexity comes for free.
   F-Rep is a volumetric representation, where functions map every point (x,y,z) in 3D space onto the properties of a shape. These properties include spatial extent, colour, material. F-Rep is a better way to program a 3D printer.

6. In the mesh world, important geometric operations like union and intersection
   are extremely complex and tricky to program. You don't implement these yourself, you use
   an expert implementation like CGAL or Carve. There are many more geometric operations available
   in open source for F-Rep than there are for meshes, and these operations are surprisingly easy
   to program. Eg, union and intersection are trivial.
   So it's practical for the entire Curv geometry library to be written in Curv itself,
   and it's much easier for users to define sophisticated new operations and distribute them
   as libraries.

7. F-Rep is well suited to being directly rendered by a GPU.
