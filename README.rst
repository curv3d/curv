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
  intersections, degenerate triangles, or flipped triangles.

Getting Started
===============
* To install the software, see `<BUILD.md>`_.
* The documentation is here: `<docs/README.rst>`_.
* Mailing list: `<https://groups.google.com/d/forum/curv>`_.
  You can join the mailing list using your Google account (or you'll be prompted to create an account).
  If you don't want to have a Google account, then send email to `doug@moens.org`
  and I will send you an invitation to join the list.
* To contribute, see `<CONTRIBUTING.md>`_.

Hardware Requirements
=====================
Two platforms are currently supported: Linux and macOS. I currently test
on Ubuntu LTS and macos 10.11. Windows support is planned but not scheduled.

Curv requires direct access to a GPU made by Intel, AMD or Nvidia, using the
vendor supplied GPU driver.

* On Linux, the Mesa open source GPU driver is currently too buggy (as of
  August 2018). You need to use the closed source vendor supplied GPU driver
  instead. For AMD GPUs, this means you need the AMDGPU-PRO driver, which is
  only officially supported on Ubuntu LTS, Red Hat and SUSE. There seem to be
  fewer problems using Intel integrated graphics and Nvidia GPUs.
* If Curv is invoked remotely via vncviewer, then it might not have direct
  access to GPU hardware.
* If Curv is run inside a VM, then it might not have direct access to the GPU.
  You need to do extra work to ensure that the VM is GPU accelerated.

..
  In the future, the geometry engine will be rewritten for scaleability
  and performance. I anticipate you will then need a GPU that supports one of
  the following standards: OpenGL 4.3, macOS Metal, DX12 or Vulkan.
  For laptops, the cutoff will be somewhere between 2012 and 2013 as the year of
  manufacture.
