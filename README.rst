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
* There are two Curv web interfaces, both in the early prototype
  stage as of Nov 2021 (limited functionality):
  * https://curv.leefallat.com
  * https://codecad.xyz then click on Curv at bottom of page.
* For the full experience, build and install the software
  (Windows, MacOS, Linux), see `<BUILD.md>`_.
* On Linux, you can use a prebuilt appimage -- click on the latest release
  in the Curv github page and scroll to the bottom.
* The documentation is here: `<docs/README.rst>`_.
* To contribute, see `<CONTRIBUTING.md>`_.

Community
=========
There is a high volume chat room.
* On matrix: https://app.element.io/#/room/!BbbouYwtinZcRySkDA:matrix.org
* And linked via 2-way bridge to the Curv Discord channel maintained
  by https://codecad.xyz (visit their site to find an invite).
* This earlier Discord channel has a ton of interesting discussion history
  but is now inactive: `<https://discord.gg/ZSPXaMNWfW>`_.

There is a Github Discussions page:
`<https://github.com/curv3d/curv/discussions>`_.

There is a low-volume mailing list: ``curv@googlegroups.com``,
connected to a web forum: `<https://groups.google.com/d/forum/curv>`_.
You can join the mailing list using your Google account (or you'll be prompted
to create an account).

Hardware and OS Requirements
============================
Linux, MacOS (intel & apple silicon) and Windows (native & WSL) are supported.

In order to export meshes (STL files, etc) using the fast ``-Ojit`` method,
you need to have a C++ compiler installed (GCC or Clang), and you need
the ``glm`` C++ math library installed. These things will already be installed
if you have built Curv from scratch using the build instructions. Otherwise,
if you are using a prebuilt binary, you may want to install these things.
Either way, see `<BUILD.md>`_.

Curv uses OpenGL 3.3.
The recommended configuration is a GPU made by Intel, AMD or Nvidia,
using a known working GPU driver (see below).

* On Linux, the GPU needs to be modern enough to be supported by the latest
  driver version from the GPU vendor. Any GPU from 2012 or later will work.
  Some older GPUs may work: check the list of supported hardware for the driver.
* On Macintosh, I recommend the current release of MacOS, or 1 or 2 releases
  earlier. More precisely, Curv works on any version of MacOS supported by
  `Homebrew <https://brew.sh/>`_. (For older systems, you would need to
  resurrect an older version of Homebrew. Also, on a pre-Metal system
  (hardware from 2011 or earlier), some Curv programs may not work correctly
  due to the GPU.)
* On Windows, you can use `MSYS2 <https://www.msys2.org/>`_ to build and run a
  native executable; see `<WINDOWS.md>`_. Alternatively, you can use
  `Windows Subystem for Linux (WSL) <https://en.wikipedia.org/wiki/Windows_Subsystem_for_Linux>`_
  for building and running a Linux executable.
* Raspberry Pi isn't supported yet.

  * Raspberry Pi 4 support is planned once the migration from OpenGL to WebGPU
    is done. Then Curv will be using Vulkan on Raspberry PI 4.
  * Raspberry Pi 3 and earlier won't be supported, as the GPU is not powerful
    enough.

* On Linux, you have 3 choices:

  * An Nvidia GPU, with the Nvidia closed source driver.
    Any GPU supported by the latest Nvidia driver will work with Curv.

    The open source Nouveau driver is not supported; it is too slow and buggy.
    Curv runs too slow, with visual glitches. See `issue #78`_.

  * An Intel GPU, using the Intel supplied open source driver (based on Mesa).
    Any GPU supported by the latest Intel driver will work with Curv
    (this means: Intel HD Graphics or later).

  * An AMD GPU with the open source Mesa driver, version 19.x or later.
    The AMDGPU-PRO (closed source) driver should work, but I have no testing
    reports for it.

    Mesa version 18.x or earlier has a bug on AMD (`issue #30`_) which prevents
    some Curv programs from running.

* If Curv is invoked within a VNC session, then it might not have direct
  access to GPU hardware (a slow software renderer is used instead of the GPU).
  Curv requires a GPU accelerated VNC server.
  Try `TurboVNC`_ combined with `VirtualGL`_.
* If Curv is run inside a VM, then it might not have direct access to the GPU.
  You need to ensure that the VM is GPU accelerated.
* You can run Curv from the command line on a headless Linux server, to export
  images and meshes. For image export, you may need to configure a dummy X
  server: see `<https://github.com/curv3d/curv/discussions/131>`_.

Curv is not as GPU intensive as AAA video games. Integrated graphics will
work fine for most stuff. But you can definitely write Curv code that will
benefit from expensive discrete graphics.

Why is Curv so picky about GPU drivers and hardware, when [some old 3D
software] runs just fine? The answer is that old 3D software relies primarily
on triangle meshes for representing and rendering 3D shapes, whereas Curv
uses signed distance fields to represent shapes. Signed distance fields are
a powerful, hot new technology that is only made practical by modern GPUs.
Curv uses shader programs to render shapes, and uses larger and more complex
shader programs than [some old 3D software]. This places a heavy and atypical
load on the GPU driver and hardware, which old hardware and old, outdated
driver software may not be prepared to deal with.

.. _`TurboVNC`: https://turbovnc.org/About/Introduction
.. _`VirtualGL`: https://virtualgl.org/About/Introduction
.. _`issue #78`: https://github.com/curv3d/curv/issues/78
.. _`issue #88`: https://github.com/curv3d/curv/issues/88
.. _`issue #30`: https://github.com/curv3d/curv/issues/30
.. _`The open source AMD driver has a bug`: https://bugs.freedesktop.org/show_bug.cgi?id=105371
