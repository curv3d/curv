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
New CSG primitives can be defined using a low level geometry interface
based on Function Representation.

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

Code for the `gyroid` primitive::

  gyroid = make_shape {
    dist(x,y,z,t) = cos(x)*sin(y) + cos(y)*sin(z) + cos(z)*sin(x),
    is_3d = true,
  }

Competing Shape Representations
===============================
There are two important classes of representation for 2D and 3D shapes:

+-------------------------------------+-----------------------------------+
| **Explicit Modelling**              | **Implicit Modelling**            |
+-------------------------------------+-----------------------------------+
| Directly generate points            | Answer questions                  |
| that comprise the shape             | about particular points           |
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

Symmetry and Space Folding
==========================

The 4th Dimension is Time
=========================

Procedural Modelling Techniques
===============================
* sweeps

  * extrude and loft
  * perimeter_extrude (sweep 2D shape along 2D implicit curve -> 3D shape)
  * isosurface (sweep circle along 2D curve, sphere along 3D curve or surface)
  * constructing implicit curves and surfaces
  
    * shell
    * MERCURY: intersection->curve
    
  * sweeping a parametric curve or surface: more expensive
  * space warp operators/fancy blending operators can be an alternative to sweeping

* morphing, blending, convolution
* deterministic fractals

  * MandelBulb3D

* fractal noise, perlin noise

  * noisy fractal solids: mountains, clouds, etc
  * perlin noise: smoke, solid textures (marble, wood)
  
* Hypertexture: engraving/perturbing the surface of a solid. An implicit modelling technique.
* Grammars, L-Systems

  * Use a context free, generative grammar to generate a complex shape, like a tree, leaf or city.
    Or fractals.
  * during the 1990's: use L-System to generate a skeleton, then flesh it out
    using F-Rep. Popular for modelling living things. See "algorithmic botany"
    and "implicit seafood" web sites.
  * idea: use a grammar to generate a tree of space folding operations: more complexity with fewer operations.

Fractal Noise
=============
.. image:: images/smoke3.png

* White noise: Each (x,y) or (x,y,z) coordinate
  is mapped to a uniformly distributed pseudo-random number in range [0...1]
  using a hash function.
  
  |white_noise|
* Value Noise: Random values are generated at lattice points.
  The noise value at a point is interpolated from nearby lattice points.
  
  |value_noise|
* Gradient noise: Random gradients are generated at lattice points. The gradient of a point
  is interpolated from the nearby lattice points. The gradient is converted to a value in [0...1].
  Smoother than value noise, with fewer grid artifacts.
  (Examples: Perlin noise, Simplex noise.)
  
  |gradient_noise|
* Fractal noise (Fractal Brownian Motion):
  Gradient noise is generated at a series of higher frequencies (different lattice spacings),
  and added together. Higher frequencies are attenuated.
  
  |fractal_noise|
* Many more types and variations of noise have been invented.

.. |white_noise| image:: images/white_noise.jpg
.. |value_noise| image:: images/value_noise.jpg
.. |gradient_noise| image:: images/gradient_noise.jpg
.. |fractal_noise| image:: images/fractal_noise.jpg

Applications: smoke, flames, clouds, mountains, solid textures like marble or wood, etc.

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

Compiling Curv to GPU Code
==========================
