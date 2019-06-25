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
on Ubuntu LTS and MacOS 10.11. Windows 10 with WSL might work, but isn't tested.

Curv uses OpenGL 3.3.
The recommended configuration is a GPU made by Intel, AMD or Nvidia,
using the vendor supplied GPU driver.

* On Macintosh, you need MacOS 10.7 or later.
  If your system supports Metal, Curv is fully supported.
  On a pre-Metal system (hardware from 2011 or earlier),
  some Curv programs may not work correctly.
* On Linux, the GPU needs to be modern enough to be supported
  by the latest driver version from the GPU vendor. Any GPU from 2012 or later
  will work. Some older GPUs may work: check the list of supported hardware
  for the driver.
* On Windows 10, you should be able to use WSL to run Curv.
  I'm waiting for a volunteer to confirm this.
* Raspberry Pi 4 should work, but I haven't tested it yet.
  It has a Broadcom VideoCore 6 (VC6) GPU, and the default Raspian Linux distro
  has a new GPU driver, the open source Mesa “V3D” driver developed by
  Eric Anholt at Broadcom.
  Raspberry Pi 3 is not supported: it has a Broadcom VC4 GPU,
  which only supports OpenGL 2.1, and Curv needs OpenGL 3.3.

* On Linux, you have 3 choices:

  * Nvidia has the best GPU hardware. I recommend the Nvidia closed source driver.
    Any GPU supported by the latest Nvidia driver will
    work with Curv.

    The open source Nouveau driver is slow and buggy. Curv runs too slow,
    with visual glitches. If you don't mind driver hacking, there are some
    mitigations you can try. See `issue #78`_.

  * An Intel GPU, using the Intel supplied open source driver (based on Mesa).
    Intel is your best choice if you want to use a driver that is free software.
    Any GPU supported by the latest Intel driver will work with Curv
    (this means: Intel HD Graphics or later).

  * An AMD GPU. I recommend using the AMDGPU-PRO (closed source) driver,
    which is only officially supported on Ubuntu LTS, Red Hat EL (not Fedora),
    and SUSE. 

    `The open source AMD driver has a bug`_ which prevents some Curv programs from running.
    If you don't mind driver hacking, read the bug report and try some of the mitigations
    suggested there. You might need to install a new driver version and/or install driver patches.

* If Curv is invoked within a VNC session, then it might not have direct
  access to GPU hardware (a slow software renderer is used instead of the GPU).
  Curv requires a GPU accelerated VNC server.
  Try `TurboVNC`_ combined with `VirtualGL`_.
* If Curv is run inside a VM, then it might not have direct access to the GPU.
  You need to ensure that the VM is GPU accelerated.

Why is Curv so picky about GPU drivers and hardware, when [some old 3D software] runs just fine?
The answer is that old 3D software relies primarily on triangle meshes for representing
and rendering 3D shapes, whereas Curv uses signed distance fields to represent shapes.
Signed distance fields are a powerful, hot new technology that is only made practical
by modern GPUs.
Curv uses shader programs to render shapes, and uses larger and more complex shader
programs than [some old 3D software]. This places a heavy and atypical load on
the GPU driver and hardware, which old hardware and old, outdated driver software
may not be prepared to deal with.

.. _`TurboVNC`: https://turbovnc.org/About/Introduction
.. _`VirtualGL`: https://virtualgl.org/About/Introduction
.. _`issue #78`: https://github.com/curv3d/curv/issues/78
.. _`The open source AMD driver has a bug`: https://bugs.freedesktop.org/show_bug.cgi?id=105371
