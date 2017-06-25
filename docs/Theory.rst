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

Code for the twisted, coloured torus::

  torus(2,1)
    >> colour (radial_rainbow 1)
    >> rotate(tau/4, Yaxis)
    >> twist 1

Code for the model "Shrek's Donut"::

  smooth_intersection .1 (
    torus(tau*2,tau),
    gyroid >> shell .1 >> df_scale .33 >> rect_to_polar (tau*6),
  ) >> colour (hsv2rgb(1/3,1,.5))

Function Representation
=======================
Internally, Curv represents geometric shapes using Function Representation (F-Rep).

In this representation, a shape contains functions that map every point (x,y,z) in 3D space onto the shape's properties, which may include spatial extent, colour, material.

F-Rep is very expressive:
shapes can be infinitely detailed, infinitely large. Any shape that can be
described using mathematics can be represented exactly.

Curv provides a low level API for defining CSG primitives using F-Rep.
Using this API, the entire CSG geometry API is defined using Curv code.

Pure Functional Programming
===========================
Curv is a pure functional language. Why?

* simple, terse, pleasant programming style
* simple semantics
* can easily be translated into highly parallel GPU code

In Curv, geometric shapes are first class values, and are constructed by transforming and combining simpler shapes using a rich set of geometric operations. This style of specification is called CSG: Constructive Solid Geometry. It's easy and pleasant to use, very expressive, and is a good match with functional programming.

good for CSG, good for F-Rep

file format

unique contribution of Curv: pure functional + csg + f-rep in one language

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

Signed Distance Fields
======================
Curv uses a specific type of F-Rep called Signed Distance Fields
for representing the spatial extent of a shape.

A signed distance field is a function f(x,y,z) which maps each point in 3-space
onto the minimum distance from that point to a boundary of the shape.
An SDF is zero for points on the boundary of the shape, negative for points
inside the shape, and positive for points outside of the shape.

A shape, plus 3 views of its SDF:

|sdf1| |sdf2|

.. |sdf1| image:: images/sdf1a.png
.. |sdf2| image:: images/sdf2a.png

|sdf3a| |sdf3b|

.. |sdf3a| image:: images/sdf3a.png
.. |sdf3b| image:: images/sdf3b.png

An SDF is differentiable almost everywhere. At the differentiable points, the slope is 1, and the gradient points towards the closest boundary. (This is useful.) The non-differentiable points are equidistant between two boundaries. The singular points that occur inside a shape are called the Skeleton or Medial Axis. (There is a technique for modelling shapes by specifying their skeleton.)

Isocurve and isosurface.

SDF Applications
================
* collision detection: https://www.youtube.com/watch?v=x_Iq2yM4FcA
* controlling a 3D printer
  
  * powder printer: XYZ raster scan, optionally with colour or material
  * plastic printer: boundary/infill

* controlling a CNC mill
* soft shadows (ambient occlusion)
* gradients and normals
* fast, scaleable font rendering
* demoscene (shadertoy.com) https://www.shadertoy.com/view/MdX3Rr
* video games

  * destructible terrain: UpVoid Miner by UpVoid
  * in game modelling: Dreams by Media Molecule https://www.youtube.com/watch?v=4j8Wp-sx5K0

Deriving an SDF
===============
derivation for simple CSG primitives
* circle
* union and intersection (cheap vs expensive)
* rigid body transforms: translate, rotate
* isotropic and anisotropic scaling

Sphere Tracing
==============

Symmetry and Space Folding
==========================

The 4th Dimension is Time
=========================

Compiling Curv to GPU Code
==========================
