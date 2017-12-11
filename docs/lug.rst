=================================================
Curv: A Language for making Art using Mathematics
=================================================
|twistor| |shreks_donut|

.. |twistor| image:: images/torus.png
.. |shreks_donut| image:: images/shreks_donut.png

What is Curv?
=============
Curv is an open source† 3D **solid modelling language**,
for **3D printing** and **procedurally generated art**.

It's **easy to use** for beginners. It's incredibly **powerful** for experts.
And it's **fast**: all rendering is performed by the GPU.

Curv is also an expressive **file format**
for the exchange of 2D and 3D procedurally generated graphics.

† github.com/doug-moen/curv -- BSD licence

How Curv Achieves these Design Goals
====================================
Ease of use:
  For ease of use, the best choice is Constructive Solid Geometry (CSG)
  embedded in a simple, pure functional language.

Rendering Speed:
  To quickly render a wide range of CSG primitives,
  the best choice is to render on a GPU,
  and to represent shapes using Signed Distance Fields (SDF)
  and Function Representation (F-Rep).

Expressive Power:
  SDF/F-Rep is the best choice for CSG:
  A wide range of CSG primitives are available in Open Source.
  New CSG primitives can be defined directly in Curv.

OpenSCAD is Amazing
===================
* Marius Kintel's April 2014 KWLUG presentation on OpenSCAD
* Procedurally generated, parametric 3D models.
  Named model parameters, conditionals, loops, user defined functions.
* Pure functional CSG: primitive shapes, operations for constructing new
  shapes from existing shapes.
* Minimal boilerplate,
  compared to 3D modelling in JavaScript, Python, etc.
* Nice GUI.

The OpenSCAD Language is Weak
=============================
The language is weak and complicated:

* 4 universes of things: values, functions, modules, shapes
* 3 namespaces
* can't use functions as arguments, return as results
* Can't store shapes in variables
* Can't query a shape, eg can't ask for bounding box.
* no record values

OpenSCAD vs Curved and Coloured Shapes
======================================
Curved surfaces & organic shapes are hard to achieve:

* Lots of triangles are needed to make curved surfaces look good,
  but this makes boolean operations (union etc) unusably slow.
* Few CSG operations for creating curved surfaces.

Very limited support for colour.

I Get Involved with the OpenSCAD Project
========================================
* 2014: series of proposals to strengthen the language.

  * Function, shape & record values. Module system.
  * Marius is supportive. The community is divided.

* OpenSCAD2 project: first commit May 2015

  * A proposal to improve the language, 100% backward compatibility.

* Problems:

  * Clean design means 1 namespace, 90% backward compatibility.
  * Which means a fork, dividing the community.
  * 100% backward compatibility is difficult to design,
    time consuming to implement, creates a lot of language complexity.
  * No time to create a better geometry kernel.

The Curv Project
================
Curv project on github: first commit May 2016

* New language: simple, orthogonal, powerful
* New geometry kernel: signed distance fields and F-Rep
* Volumetric colour (F-Rep)
* Curv programs compile into GPU code (fast rendering)

Language Goals
==============
* Easy to use for novices and artists. Focus is artistic exploration,
  not software engineering.
* Powerful enough for experts. Libraries of new CSG operations can
  be written in Curv and distributed over the internet.
* Safe and secure. A Curv program downloaded over the internet can't
  encrypt all your files or exfiltrate personal information to a server.

Language Characteristics
========================
* Simple, elegant, powerful: One universe, one namespace.
* Dynamically typed.
* Pure functional language. No I/O. Programs are expressions.
* Curried functions, patterns, list/record/string comprehensions.
* Array language: scalar arithmetic operations work on
  vectors, matrices and higher dimensional arrays.
* 7 data types: the 6 JSON types, plus function values.
* Superset of JSON.
  Curv is a data interchange format for pure functional data.

The 7 Data Types
================
* ``null``
* Boolean values: ``true`` and ``false``
* Numbers: ``42``, ``3.1416``
* Lists: ``[1,2,3]`` or ``(1,2,3)``, ``1..10``
* Strings: ``"hello, world"``
* Records: ``{angle: 90*deg, axis: Z_axis}``
* Functions: ``x -> x + 1``

Function Call Syntax
====================
* Function call is a binary operation: ``f x``
* Argument lists: ``f (x,y)``
* Argument records: ``rotate {angle: a, axis: v}``
* Curried functions: ``f x y``
* Pipelines: ``colour red (rotate (45*deg) (cube 10))``
  becomes ``cube 10 >> rotate (45*deg) >> colour red``
* Infixes: ``union(cube, cylinder {h: 5, d: .5})``
  becomes ``cube `union` cylinder {h: 5, d: .5}``

Pipeline Example
================
::

    dodecahedron
     >> colour red
     `union` icosahedron

|dodeca-icosa|

.. |dodeca-icosa| image:: images/dodeca-icosa.png

Definitions and Blocks
======================
Definitions::

  pi = 3.1416
  incr x = x + 1

Blocks::

  let diameter = 1;
      sticklen = 4;
  in union (
      sphere(diameter) >> colour red,
      cylinder{h:sticklen, d:diameter/8} >> move(0,0,-sticklen/2)
  )

  fact 6 where fact n = if (n==0) 1 else n*fact(n-1)

Libraries
=========
Contents of "lollipop.curv"::

  {
  lollipop(diameter, sticklen) =
      union (
          sphere(diameter) >> colour red,
          cylinder{h:sticklen, d:diameter/8} >> move(0,0,-sticklen/2)
      );
  }

To use the library elsewhere::

  lollipop(1,4)
  where
  include file "lollipop.curv";

The Shape Library
=================
* written entirely in Curv
* shapes are records
* 2D and 3D primitive shapes
* boolean (set theoretic) operations
* transformations: affine, non-affine, 2D->3D, repetition
* distance field ops: offset, shell, morph, blend
* colour

Primitive Shapes
================
2D:

* circle, ellipse, stroke
* square, rect, regular_polygon, convex_polygon, half_plane

3D:

* sphere, ellipsoid, cylinder, cone, torus, capsule
* tetrahedron, cube, octahedron, dodecahedron, icosahedron
* box, prism, pyramid, half_space

Transformations
===============
* Affine: move, rotate, reflect, scale, shear
* Non-affine: taper, twist, bend
* 2D->3D: extrude, revolve
* 3D->2D: slice
* Repetition: repeat_x, repeat_xy, repeat_xyz, repeat_mirror, repeat_radial

Additive/Subtractive Operations
===============================
* Boolean: complement, union, intersection, difference
* offset, shell, morph, blend

Colour
======
* A **colour field** maps every point in space onto a colour.
* Every shape has a colour field.
* Standard support for constructing colour gradients.
* Colour field transformations.
* Colour blending (in morph, blend, union)

Shape Representations
=====================
+-------------------------------------+----------------------------------------+
| **Explicit Modelling**              | **Implicit Modelling**                 |
+-------------------------------------+----------------------------------------+
| Directly generate boundary points   | Answer questions                       |
|                                     | about particular points                |
+-------------------------------------+----------------------------------------+
| parametric equation (unit circle):: | implicit equation (unit circle)::      |
|                                     |                                        |
|  (x,y) = (cos t, sin t)             |   x^2 + y^2 - 1 = 0                    |
+-------------------------------------+----------------------------------------+
| **Boundary Representations**        | **Volumetric Representations**         |
+-------------------------------------+----------------------------------------+
| parametric splines                  | simple vs Signed Distance Fields       |
+-------------------------------------+----------------------------------------+
| triangle mesh                       | discrete (voxels) vs continuous        |
|                                     | (Function Representation)              |
+-------------------------------------+----------------------------------------+

Benefits of SDF/F-Rep
=====================
Curv 0.0 uses Signed Distance Fields with Function Representation.

* Exact representation of many shapes (curves, fractals)
* Infinitely large & infinitely detailed objects
* Large # of operations available (bend, twist, blend, etc)
* Fast boolean operations
* F-Rep for colour: gradients, quasi-photographic imagery
* Can describe 3D-printable objects not describable using STL or OpenSCAD.
  More efficient 3D printing.

How SDF/F-Rep Works
===================
A signed distance field maps each point in space
onto the minimum distance from that point to the boundary of the shape.

An SDF is zero for points on the boundary of the shape, negative for points
inside the shape, and positive for points outside of the shape.

The ``circle`` primitive is defined like this::

  circle r = make_shape {
    dist(x,y,z,t) = sqrt(x^2 + y^2) - r;
    bbox = [[-r,-r,0], [r,r,0]];  // axis aligned bounding box
    is_2d = true;
  };

..
  Sphere Tracing
  ==============

Future Work
===========
* Release 0.0 is coming soon: documentation, STL export
* text, animation, fractals, noise, splines, sweep, Voronoi, Conway polyhedron operators, ...
* Reference external libraries via URL
* Import images, STL
* Geometry engine: compiler, discrete SDFs, ...
* Export colour models for 3D printing
* GUI, tweak model parameters using slider

Live Demo
=========
