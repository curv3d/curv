High Level Geometry Interface (Wishlist)
========================================
* ellipse, ellipsoid.
* pyramid
* antiprism
* Twist, Taper, Bend.
* 2D->3D: cylinder_extrude, stereographic_extrude, ...
* A properly designed set of colour operations, with multiple colour spaces,
  and colour blending.
* Animation operators (depends on the new geometry compiler).
* Easing functions.
* Noise functions.
* Conway polyhedron operators.
* Spline curves and surfaces.
* A comprehensive set of CSG operators for constructing fractals,
  similar to Mandelbulber.

Major Features I'd Like for Release 1.0
=======================================
Documentation
-------------
* tutorial/getting started
* curv tool, reference
* core language, reference
* high level geometry api, reference
* low level geometry api, reference

User Interface
--------------
Unified GUI
  with edit window, graphics window, console. Eg, like OpenSCAD.
  Editor like The Book Of Shaders (use sliders to tweak numerals in source).

Interactive Debugger
  with CLI interface, invoked when a Curv program aborts with an error.

Import/Export
-------------
* STL export
* Image import
* Animated GIF export
* CSG Tree import/export

High Level Geometry Interface
-----------------------------
* Finalize and document the v1.0 CSG interface.
* CSG operations can take a shape constructing function as argument,
  and invoke it from a distance function. Needed for animation API, etc.
  Depends on rewrite of geometry compiler.
* Pluggable renderer and parameterized lighting model.

Core Language
-------------
* Finalize and document the core language design.
* Function definitions with record patterns (for CSG library)
* Shape/Record Constructors. A shape or record value can optionally remember
  the name of its constructor and constructor arguments. (for CSG tree import/
  export)
* Shape/Record Prototypes?

Low Level Geometry Interface & Geometry Engine
----------------------------------------------
GL Optimizer
  A compile time optimizer and partial evaluator, which is run before
  generating GLSL (GPU) code. This will increase rendering performance,
  but it will also increase the range of language constructs that are
  allowed in distance functions, needed for CSG interface.

Mesh Generator
  OpenGL compute shader that converts a shape to a triangle mesh.
  Needed for STL export.

Build System
------------
* Geometry Unit Tests
* Continuous Integration Build Server. Travis-CI.org

Future Ideas/Research Projects
==============================

User Interface
--------------
Node-Based Visual Programming Language
  with nodes that contain Curv code. See Antimony, Max, etc.

GUI Settable Model Parameters
  with language support to declare parameters, like OpenSCAD customizer.
  If we don't get a book-of-shaders editor in 1.0.

Import/Export
-------------
* SVG import/export

Geometry Engine
---------------
* Text.
  Look at Qt GPU text renderer, which converts text to an SDF.

* Fast union operator (sublinear execution time).

  * Eg, hierarchical SDFs.
  * Eg, recursive subdivision and interval arithmetic in a compute shader?
  * Eg, cache expensive subshapes, eg in a 2D/3D texture distance field.

* Support for non-Lipshitz distance functions.

  * Eg, recursive subdivision and interval arithmetic in a compute shader.

* Efficient STL Import.

  * Eg, convert STL to a fast function rep using a slow offline process,
    like FastRBF or equivalent.
