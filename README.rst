=================================================
Curv: a language for making art using mathematics
=================================================

By Doug Moen <doug@moens.org>

|twistor| |shreks_donut|

.. |twistor| image:: docs/images/torus.png
.. |shreks_donut| image:: docs/images/shreks_donut.png

Curv is a programming language for creating art using mathematics.
It's a 2D and 3D geometric modelling tool that supports full colour,
animation and 3D printing.

Features:

* Curv is a simple, powerful, dynamically typed, pure functional
  programming language.
* Curv is easy to use for beginners. It has a standard library of
  predefined geometric shapes, plus operators for transforming and
  combining shapes. These can be plugged together like Lego to make 2D and 3D
  models.
* Coloured shapes are represented using Function Representation (F-Rep).
  They can be infinitely detailed, infinitely large, and any shape or colour
  pattern that can be described using mathematics can be represented exactly.
* Curv exposes the full power of F-Rep programming to experts.
  The standard geometry library is written entirely in Curv.
  Many of the demos seen on shadertoy.com can be reproduced in Curv,
  using shorter, simpler programs. Experts can package techniques used on
  shadertoy as high level operations for use by beginners.
* Rendering is GPU accelerated. Curv programs are compiled into fragment
  shaders which are executed on the GPU.
* Curv can export meshes to STL and OBJ files for 3D printing.
  These meshes are defect free: watertight, manifold, with no self
  intersections or degenerate triangles.

History and Rationale
=====================
Curv builds on earlier work:

Haskell (1998)
  Shows that pure functional programming has awesome
  benefits, and is a viable alternative to imperative style programming.

  *BUT*: Haskell has a very high learning curve, and maybe that's why
  pure functional programming hasn't become more mainstream.
  Someone should build an easy to use pure functional language
  for beginners, with a simple dynamic type system.

Processing (2001)
  A compelling programming language for beginners:
  learn how to program by writing code that makes art.
  A simple GUI with code on the left, the picture it generates on the right.

  *BUT*: Processing is an old fashioned, low level, imperative language.
  It's a 1990's vision of how to write graphical code.

OpenSCAD (2010)
  Similar to Processing, but with a nicer language:
  it's a high level declarative language that manipulates 2D and 3D shapes
  as objects, using powerful operations to transform and combine shapes.
  And, you can export shapes for 3D printing.

  *BUT*: It's slow, and lacks expressive power.
  
  * The geometry engine is based on polyhedral meshes: it's slow and memory
    intensive. Complex models with many triangles are expensive.
    Boolean operations are expensive. Doesn't use the GPU for geometry
    computations.
  * The language lacks expressive power. Functions and shapes are not first
    class values. You can't query the properties of a shape. Very little
    support for colour.

ImplicitCAD (2012)
  A clone of OpenSCAD that uses F-Rep (Function
  Representation) instead of meshes to represent shapes. It provides a glimpse
  of the greater expressive power of F-Rep. Functions are values, so you
  can do functional programming.

  *BUT*: Still a very limited language. You can't define new
  function representations within the language, so most of the potential
  of F-Rep geometry is inaccessible. And it doesn't use the GPU to speed up
  geometry processing.

ShaderToy.com (2013)
  A website with a Processing-like interface
  for doing F-Rep programming using WebGL and the GLSL language. Showcases the
  full power of F-Rep programming on a GPU.

  *BUT*: GLSL is a very low level and limited language. The programming
  model is very difficult. You can't 3D print the models you see on ShaderToy.

Curv combines the best features of its predecessors. It's a high level,
pure functional language for geometric art and 3D printing.
It is easy for beginners to get started with, but it also exposes the full
power of F-Rep programming to experts. Experts can package techniques seen on
ShaderToy as high level operations with a simple API, so that beginners can
create the kind of effects seen on ShaderToy using much shorter, simpler
programs. And you can 3D print your models. All rendering is GPU accelerated.

Project Status
==============
The current release is 0.1: "mesh export":

* The language and shape library are stable, except where documented otherwise.
* STL and OBJ file export (for 3D printing) is now implemented.
* STL file import and a Windows port are still in the future.
* To install the software, see `<BUILD.md>`_.
* The documentation is here: `<docs/README.rst>`_.
* To contribute, see `<CONTRIBUTING.md>`_.

Hardware Requirements
=====================
Two platforms are currently supported: Ubuntu 16.04 and macOS.

Curv is sensitive to the GPU hardware capabilities and the graphics driver.
Good hardware and good driver software are a requirement.
If the website `<http://shadertoy.com>`_ doesn't work in the current version
of FireFox, then Curv won't work well either.
On Linux, if the open source driver isn't cutting it, try the proprietary driver.

Check for current GPU issues: `<https://github.com/doug-moen/curv/issues?q=is%3Aissue+is%3Aopen+label%3AGPU>`_
