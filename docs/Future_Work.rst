Future Work
===========
These are possible ideas for future work and new features.

Current Priorities
------------------
Downloadable binaries for Linux, macOS, Windows.
  So you don't need to build the program from source.
  Add a Windows port, because lots of potential users are there.

``gcurv`` GUI
  A 3 pane GUI program similar to OpenSCAD or libfive Studio.
  So you don't need the CLI to launch Curv.

STL file export
  So you can 3D print your models. Look at: libfive mesh generation.
  Ideally, generate the mesh on the GPU using OpenCL: should be much faster.

STL file import
  A two step process: Use a separate program to convert the mesh to a
  discrete SDF file. Then import the SDF file from a Curv program.
  Look at: mTec.

Optimizing shape compiler
  Generates optimized GLSL code for faster rendering.
  Extends the subset of Curv that can be used to write distance functions.

Optimized geometry pipeline
  Instead of generating a monolithic fragment shader,
  generate a pipeline of shaders and compute kernels,
  and use optimizations from mTec. Faster, more scaleable rendering.
  But note: mTec requires OpenGL 4.3, macOS is stuck at OpenGL 4.1.
  If feasible, geometry engine should support "basic" and "advanced"
  modes so that macOS is still supported.

Documentation
-------------
* Tutorial and cookbook.
* The reference manual uses images to illustrate each graphics primitive.
* Interactive documentation.

  * Eg, `<shapes/Colour.rst>`_ contains an interactive colour picker for each
    colour space.
  * Eg, the shape library documentation contains interactive images of the results
    of each graphics primitive, where you can tweak model parameters in each example
    using sliders.
  * Like in the Book of Shaders, https://thebookofshaders.com/

The Shape Library
-----------------
* Tools for visualizing models for 3D printing.

  * Eg, colouring the surface, based on Z component of the
    surface normal, to show areas of the model with too much overhang.

* See `<shapes/Future_Work.rst>`_, and also the "Future Work" sections in individual shape library topics.

GUI
---
* A 3 panel OpenSCAD-like GUI (editor, console, graphics pane). http://www.openscad.org/
* 4th panel is a browser, for discovering shape operations, using search or by navigating a classification tree.
* A "Book of Shaders" style editor: you can tweak model parameters
  graphically, directly in the source code view. https://thebookofshaders.com/
  and https://github.com/patriciogonzalezvivo/glslEditor
* IDE-style auto completion in editor.

Package Manager
---------------
* A program may import Curv values from the internet by specifying a package URL,
  package version, relative pathname. Similar to Rust Cargo.
* Use google to find new Curv packages on the internet.
  Packages may be manually installed, or implicitly installed by being dependencies
  of another package.
* The GUI browser lets you browse installed packages.

CLI
---
* An interactive command line debugger.

Geometry Kernel
---------------
* STL export, for 3D printing.
* Animated GIF export.
* Offline conversion of STL files to discrete SDF files,
  plus import of discrete SDF files.
* Image file import.
* Performance improvements.

  * An optimizing GL compiler, which outputs optimized GLSL/OpenCL code.
  * A language primitive for rendering a shape to a discrete SDF object.
    Can be used to speed up interactive rendering if a subshape has an
    expensive distance field.
  * Can a GPU graphics pipeline speed up rendering? Some projects that
    use signed distance fields are known to do this for performance, eg:
    
    * Dreams by Media Molecule: https://www.youtube.com/watch?v=u9KNtnCZDMI
    * mTec: https://github.com/xx3000/mTec/blob/master/Mroz_DistanceFields.pdf
  
  * Investigate recursive subdivision and interval arithmetic as an alternative
    (or adjunct?) to sphere tracing. Like Matt Keeter's libfive, except in a compute shader.

Language Integration
--------------------
* Create a Python API for using Curv.
