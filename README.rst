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
* Curv can export meshes to STL, OBJ and X3D files for 3D printing.
  The X3D format supports full colour 3D printing (on Shapeways.com, at least).
  These meshes are defect free: watertight, manifold, with no self
  intersections or degenerate triangles.

Getting Started
===============
* To install the software, see `<BUILD.md>`_.
* The documentation is here: `<docs/README.rst>`_.
* Mailing list: `<https://groups.google.com/forum/#!forum/curv>`_.
* To contribute, see `<CONTRIBUTING.md>`_.

Hardware Requirements
=====================
Two platforms are currently supported: Ubuntu 16.04 and macOS.

Currently, Curv runs on just about any GPU.

In the future, the geometry engine will be rewritten for scaleability
and performance. I anticipate you will then need a GPU that supports one of
the following standards: OpenGL 4.3, macOS Metal, DX12 or Vulkan.
For laptops, the cutoff will be somewhere between 2012 and 2013 as the year of
manufacture.
