What is Curv?
=============
|twistor| |shreks_donut|

.. |twistor| image:: images/torus.png
.. |shreks_donut| image:: images/shreks_donut.png

Curv is an open source 3D solid modelling language, oriented towards 3D printing, procedurally generated art, and mathematical visualization.

It's easy to use for beginners. It's incredibly powerful for experts.
And it's fast: all rendering is performed by the GPU.

Thesis
======
Ease of use:
  For ease of use, the best choice is Constructive Solid Geometry (CSG)
  embedded in a simple, pure functional language.

Rendering Speed:
  To quickly render a wide range of CSG primitives,
  the best choice is to render on a GPU,
  and to represent shapes using Function Representation (F-Rep).

Expressive Power:
  F-Rep is the best choice for CSG:
  A wider range of CSG primitives are available in Open Source for F-Rep
  than for competing representations.
  
  To boost expressiveness,
  the Curv language permits experts to define new CSG primitives using F-Rep.
  (Therefore, Curv functions must be compilable into GPU code.)
  
  F-Rep is the most expressive representation for controlling a 3D printer.
  It can describe 3D-printable objects that can't be modeled by competing representations.

Constructive Solid Geometry
===========================
In a Constructive Solid Geometry (CSG) system,
there is a set of primitive shapes,
and operations for constructing new shapes
by transforming and combining simpler shapes.
The operations normally include boolean operations like union, intersection and difference,
and affine transformations like translate, scale and rotate.

Curv uses CSG as its high level geometry interface,
and provides a rich set of predefined shapes and operations.

Curv is a pure functional language in which shapes are first class values,
and CSG operations are functions that map shapes onto shapes.

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

Code for the `gyroid` primitive::

  gyroid = make_shape {
    dist(x,y,z,t) = cos(x)*sin(y) + cos(y)*sin(z) + cos(z)*sin(x),
    is_3d = true,
  }

Pure Functional Programming
===========================
Curv is a pure functional language, with both shapes and functions as first class values.
Why?

* simple, terse, pleasant programming style
* simple semantics
* can easily be translated into highly parallel GPU code
* good match for CSG and F-Rep
* security

Curv can be considered a file format for representing arbitrary geometric shapes
and distributing them across the internet. One requirement for such a file format
is security: when you open a shape file, you don't want the shape file to encrypt
all of your files and display a ransom message. Curv is not a general purpose
programming language. It doesn't have side effects, it can only compute values.
So it meets this requirement.

Unique contribution of Curv: pure functional + CSG + F-Rep in one language.

Competing Shape Representations
===============================
There are two important classes of representation for 2D and 3D shapes:

+-------------------------------------+-----------------------------------+
| **Explicit Modelling**              | **Implicit Modelling**            |
+-------------------------------------+-----------------------------------+
| Directly generate boundary points   | Answer questions                  |
|                                     | about particular points           |
+-------------------------------------+-----------------------------------+
| parametric equation (unit circle):: | implicit equation (unit circle):: |
|                                     |                                   |
|  (x,y) = (cos t, sin t)             |   x^2 + y^2 - 1 = 0               |
+-------------------------------------+-----------------------------------+
| **Boundary Representations**        | **Volumetric Representations**    |
+-------------------------------------+-----------------------------------+
| parametric splines                  | function representation           |
+-------------------------------------+-----------------------------------+
| triangle mesh                       | pixels (2D), voxels (3D)          |
+-------------------------------------+-----------------------------------+

These two classes have different strengths and weaknesses.
Certain operations that are cheap for one class are expensive
for the other class (and vice versa).

Conversions between the two classes are non-trivial:

* It's expensive to convert between parametric and implicit equations.
* It's expensive to convert between B-Rep and F-Rep.

Curv chooses F-Rep over B-Rep, but an engineering tradeoff is involved.

If you only know B-Rep procedural modelling, then learning F-Rep
requires you to think different if you want to write efficient programs.

F-Rep > Meshes
==============
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

So Why Do People Use Meshes?
============================
Historical reasons. The first consumer GPUs (1999) were designed to render meshes efficiently,
and did not support F-Rep at all. F-Rep had been used
by the movie industry since the 1980's, but was then far too expensive for real-time.

The video game industry drove the consumer GPU industry, and of course they standardized
on mesh representations. Today, all of the important games, game engines and dev tools use meshes
as the primary shape representation,
and that's why meshes are dominant. Modern games use F-Rep in a secondary role,
eg, for adding special effects to meshes.

For pure, meshless F-Rep to be practical for games, we need:

* GPUs with programmable pixel shaders (2001)
* Shader harder that is fast enough to support real time ray tracing of F-Rep (mid-2000's to present)
* Shader programming techniques that are good enough
  (mid-2000's to the present, driven by the demo scene)
* A competitive F-Rep game engine is developed. (Still waiting. But see "Dreams", still unreleased.)
* A "killer app" to justify switching technologies.
  Destructible terrain and in-game modelling have been proposed as benefits,
  both based on cheap boolean CSG operations.

Trailer for "Dreams" by Media Molecule: https://www.youtube.com/watch?v=4j8Wp-sx5K0

Signed Distance Fields
======================
Curv uses a specific type of F-Rep called Signed Distance Fields
for representing the spatial extent of a shape.

A signed distance field is a function which maps each point in space
onto the minimum distance from that point to the boundary of the shape.
An SDF is zero for points on the boundary of the shape, negative for points
inside the shape, and positive for points outside of the shape.

A 2D shape, plus 3 views of its SDF:

|sdf1| |sdf2|

.. |sdf1| image:: images/sdf1a.png
.. |sdf2| image:: images/sdf2a.png

|sdf3a| |sdf3b|

.. |sdf3a| image:: images/sdf3a.png
.. |sdf3b| image:: images/sdf3b.png

An SDF is differentiable almost everywhere. At the differentiable points, the slope is 1, and the gradient points towards the closest boundary. (This is useful.) The non-differentiable points are equidistant between two boundary regions. The singular points that occur inside a shape are called the Skeleton or Medial Axis. (There is a technique for modelling shapes by specifying their skeleton.)

Isocurve and isosurface.

SDF Applications
================
* collision detection: https://www.youtube.com/watch?v=x_Iq2yM4FcA
* controlling a 3D printer
  
  * powder printer: XYZ raster scan, optionally with colour or material
  * plastic printer: boundary/infill

* controlling a CNC mill (offsetting)
* soft shadows (ambient occlusion)
* gradients and normals
* fast, scaleable font rendering
* demoscene (shadertoy.com) https://www.shadertoy.com/view/MdX3Rr
* video games

  * destructible terrain: UpVoid Miner by UpVoid
  * in game modelling: Dreams by Media Molecule https://www.youtube.com/watch?v=4j8Wp-sx5K0

The Circle
==========
Implicit equation for a circle of radius ``r``::

  x^2 + y^2 = r^2

If we rearrange this to::

  x^2 + y^2 - r^2 = 0

then we have an implicit function that is zero on the boundary of the circle,
negative inside the circle, and positive outside the circle.
Although this is a Function Representation for a circle, it's not a Curv-compatible SDF
because the function value at p
is the square of the distance from p to the origin, not the Euclidean distance.

We fix this by further transforming the equation::

  sqrt(x^2 + y^2) = r
  sqrt(x^2 + y^2) - r = 0

and now we have a proper Euclidean SDF.

A Curv circle implementation::

  circle r = make_shape {
    dist(x,y,z,t) = sqrt(x^2 - y^2) - r,
    ...
  }

Moral: Converting an implicit equation to an SDF requires care.
Typically, you will plot the candidate distance field, look for places where
the gradient isn't 1, and construct an inverse transformation that maps 0 to 0
(leaving the boundary alone), but modifies the field at other points so that the
gradient becomes 1.

Boolean Operations
==================
A cheap way to find the union of two shapes
is to compute the minimum of their distance fields::

  union(s1,s2) = make_shape {
    dist p = min(s1.dist p, s2.dist p),
    ...
  }

Union of a square and circle:

.. image:: images/union1.png

The resulting SDF is correct for any points outside of the shape, or at the boundary.
But the SDF is incorrect inside the shape, in this case within the region where the circle and square intersect.
In this region, the SDF underestimates the distance from p to the boundary.

This approximation is okay in most cases:

* The ray tracer still works if the SDF underestimates the distance.
* Usually we only care about the SDF on the outside of a shape.

It's possible to compute an exact Euclidean union, but it's more expensive
(meaning rendering becomes slower), and it's usually not worth the price.

We amend our definition of a Curv-compatible SDF so that it is okay if the SDF
underestimates the distance. In formal math language, an SDF must be Lipshitz Continuous,
with a Lipschitz Constant of 1 (ie, don't have any distance gradient larger than 1).

Intersection can be computed using ``max``.

The complement operation negates the distance field (and converts finite shapes into infinite ones).

Transformations
===============
A transformation warps or transforms a shape in some way, by warping or transforming the
coordinate system in which it is embedded. The affine transformations are the most familiar
(translate, rotate, scale, etc) but any coordinate transformation is possible.

Translation::

  translate (dx,dy,dz) S = make_shape {
    dist(x,y,z,t) = S.dist(x-dx,y-dy,z-dz,t),
    ...
  }

To apply an affine transformation to a shape S, the transformation's distance function ``dist(p)``
performs the inverse of the transformation to the argument p before passing it to ``S.dist``.

For distance-preserving or rigid transformations (translate, rotate and reflect), that's all you need.
Otherwise, for non-rigid transformations (like scale, shear or twist),
the resulting distance field will be messed up, and needs to be fixed.

For isotropic scaling, fixing the distance field is easy::

  isoscale k S = make_shape {
    dist(x,y,z,t) = S.dist(x/k, y/k, z/k, t) * k,
    ...
  }

For anisotropic scaling, fixing the distance field requires an approximation::

  scale(kx, ky, kz) S = make_shape {
    dist(x,y,z,t) = S.dist(x/kx, y/ky, z/kz, t) * min(kx, ky, kz),
    ...
  }

Fixing the distance field can sometimes be tricky.
If you can put an upper bound D on the derivative of the broken distance field,
then divide the distance field by D and that's probably good enough.
If there's no upper bound, you need a more complicated fix.

Symmetry and Space Folding
==========================

The 4th Dimension is Time
=========================

Morphing, Blending and Convolution
==================================
Morphing from one shape to another is easy:
linear interpolation between two distance fields.

Convolution:
In Photoshop, there are image processing filters that blur or sharpen an image.
In the mathematics of image processing, this is called convolution.
Convolutions can also be applied to 3D shapes. Blurring a shape removes high
frequency components, causing sharp edges to melt, and T-junctions to be filled in.

Sweep
=====
  * extrude and loft
  * perimeter_extrude (sweep 2D shape along 2D implicit curve -> 3D shape)
  * isosurface (sweep circle along 2D curve, sphere along 3D curve or surface)
  * constructing implicit curves and surfaces
  
    * shell
    * MERCURY: intersection->curve
    
  * sweeping a parametric curve or surface: more expensive
  * space warp operators/fancy blending operators can be an alternative to sweeping

Procedural Modelling Techniques
===============================
* Hypertexture: engraving/perturbing the surface of a solid. An implicit modelling technique.
* Grammars, L-Systems

  * Use a context free, generative grammar to generate a complex shape, like a tree, leaf or city.
    Or fractals.
  * during the 1990's: use L-System to generate a skeleton, then flesh it out
    using F-Rep. Popular for modelling living things. See "algorithmic botany"
    and "implicit seafood" web sites.
  * idea: use a grammar to generate a tree of space folding operations: more complexity with fewer operations.

Fractals
========
For large or deeply iterated 3D fractals,
F-Rep wins over other representations like triangle meshes or voxels:
they require too much memory,
and performing CSG operations like union or intersection on these
bulky representations is too time consuming.

For the 3D fractal art community, F-Rep is the technology of choice,
using tools like MandelBulb3D, which are phenomenally rich and powerful.
In principle, the same models can be written in Curv.

.. image:: images/holy_box_fractal.jpg

https://www.youtube.com/watch?v=OW5RnrlTeow

Fractal Noise
=============
A noise function maps each point in 2D or 3D space onto a pseudo-random noise value in the range [0...1].

Fractal noise is a popular noise function, good for simulating natural phenomena
like smoke, flames, clouds, mountains, and solid textures like marble or wood.

Here's a 3D solid texture I hacked together in Curv using fractal noise:

.. image:: images/smoke3.png

* White noise: Each (x,y) or (x,y,z) coordinate
  is mapped to a uniformly distributed pseudo-random number
  using a hash function.
  
  |white_noise|
* Value Noise: Random values are generated at lattice points.
  The noise value at a point is interpolated from nearby lattice points.
  
  |value_noise|
* Gradient noise: Random gradients are generated at lattice points. The gradient of a point
  is interpolated from the nearby lattice points. The gradient is converted to a noise value.
  Smoother than value noise, with fewer grid artifacts.
  (Examples: Perlin noise, Simplex noise.)
  
  |gradient_noise|
* Fractal noise (Fractal Brownian Motion):
  Gradient noise is generated at a series of higher frequencies (different lattice spacings),
  and added together. Higher frequencies are attenuated.
  
  |fractal_noise|

Many more noise functions have been invented.

.. |white_noise| image:: images/white_noise.jpg
.. |value_noise| image:: images/value_noise.jpg
.. |gradient_noise| image:: images/gradient_noise.jpg
.. |fractal_noise| image:: images/fractal_noise.jpg

Sphere Tracing
==============

Hierarchical SDFs
=================
Naive: cost (N-ary union) = sum of the costs of the N arguments. Too expensive for large N.

Smart: partition space into disjoint subspaces. Maybe use multiple levels or a tree structure.
During SDF evaluation, first determine what subspace you are in (eg by walking the tree),
then evaluate the SDF for that subspace.

Can be done manually, using F-Rep API, but nicer to do it automatically. Eg,

Dreams by Media Molecule https://www.youtube.com/watch?v=4j8Wp-sx5K0

Shape Values in Curv
====================
In Curv, a shape value is represented by a record, with fields:

* ``dist`` is a function mapping ``(x,y,z,t)`` onto a signed distance value.
* ``colour`` is a function mapping ``(x,y,z,t)`` onto a colour (an RGB triple).
* ``bbox`` is an axis aligned bounding box, since this is expensive to compute from the distance function.

In the future, I'd like to support multiple shape subclasses,
with specialized CSG operations that work only on shape subtypes.
For example, I'd like to implement the Conway polyhedron operators
(which transform one polyhedron into another). Polyhedrons will contain
vertex/edge/face information.

Compiling Curv to GPU Code
==========================
The Geometry Compiler translates a shape to GPU code for rendering that shape.

For rendering on a display, the shape's distance and colour functions
are compiled into an OpenGL fragment shader.
In future, for converting a shape to a triangle mesh,
the distance function will be compiled to an OpenCL or CUDA compute kernel.
(I could also target the DirectX (Windows), Metal (macOS) and Vulkan APIs.)

Whatever the format, GPU compute kernels are written in a primitive
subset of C which lacks recursive functions and memory allocation,
and has limited support for pointers and global variables.
If I target WebGL, there is only limited support for iteration.

Here's how GPU code generation works:

* Evaluate a Curv program, producing a shape value.
* Extract the ``dist`` and ``colour`` functions, which are closures.
* Partially evaluate the body of the closure,
  treating non-local variables captured by the closure as compile time constants,
  folding constant subexpressions, and optimizing.
* Function calls are inline expanded to eliminate recursion and polymorphism,
  and enable more partial evaluation.
* The resulting transformed code is restricted to a statically typed
  subset of Curv called "GL", which can be compiled into GPU code.
* A distance function can use operations and data types that are not part of GL,
  as long as those subexpressions are partially evaluated into something that
  is supported.

As I extend the F-Rep API to make Curv faster and more powerful,
the GL subset of Curv is growing to embed an increasingly larger subset of the GLSL shader language.
