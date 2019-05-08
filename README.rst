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
Two platforms are currently supported: Linux and Macintosh. I currently test
on Ubuntu LTS and MacOS 10.11. Windows support is planned but not scheduled.

Curv uses OpenGL 3.3.
It requires direct access to a GPU made by Intel, AMD or Nvidia, using the
vendor supplied GPU driver.
On Macintosh, you just need MacOS 10.7 or later.
On Linux, the GPU needs to be modern enough to be supported
by the latest driver version from the GPU vendor. Any GPU from 2012 or later
will work. Some older GPUs may work: check the list of supported hardware for the driver.

* On Linux, you have 3 choices:

  * Nvidia has the best GPU hardware. You will need to use the Nvidia closed source driver,
    not the open source Nouveau driver. Any GPU supported by the latest Nvidia driver will
    work with Curv. Eg, see this supported hardware list:
    https://www.geforce.com/drivers/results/137276
  * An Intel GPU, using the Intel supplied open source driver (based on Mesa).
    Intel is your choice if you want to use a driver that is free software.
    Any GPU supported by the latest Intel driver will work with Curv.
    You need Intel HD Graphics -- earlier GPU technology is not supported.
  * An AMD GPU, using the AMDGPU-PRO (closed source) driver,
    which is only officially supported on Ubuntu LTS, Red Hat EL (not Fedora),
    and SUSE. Unfortunately, the open source AMD driver (based on Mesa) is too buggy
    to work with Curv right now. Your choice of Linux distro is very restricted with AMD.

* If Curv is invoked within a VNC session, then it might not have direct
  access to GPU hardware. Curv requires a GPU accelerated VNC server.
  Try `TurboVNC`_ combined with `VirtualGL`_.
* If Curv is run inside a VM, then it might not have direct access to the GPU.
  You need to ensure that the VM is GPU accelerated.

.. _`TurboVNC`: https://turbovnc.org/About/Introduction
.. _`VirtualGL`: https://virtualgl.org/About/Introduction
