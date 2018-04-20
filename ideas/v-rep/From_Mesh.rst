Importing a Mesh
================

Here's a fast path to implementing this.

* Represent a mesh using a distance grid.
  This is the simplest choice on the GPU, where 3D textures
  have built-in support in fragment shaders. Any other data
  structure is far more work to implement.
* On the CPU, represent a distance grid using OpenVDB.
* In Curv language, ``mesh_file{path="foo.stl",vsize=.1}``
  imports a mesh, converts it internally to a distance grid.

  * This can be slow. If that's a problem,
    then optimize. Write a tool, mesh-to-sdf, that converts a mesh
    to a distance grid file. Call the file format "cvox" for now.
  * The mesh conversion parameters are supplied to mesh-to-sdf,
    so a cvox file can be loaded without parameters.
  * Then I'll need a CPU implementation of a distance function for a cvox
    voxel array. I won't use openvdb at runtime. That should be okay.
  * For performance reasons, I'll want to cache voxel arrays across
    Curv compiles. Both in CPU and in GPU.
  * And then I'll want a command for flushing the cache, accessible from
    live editing mode. How? Either a menu bar on the viewer window,
    or there's an interactive CLI in the console window.
    (Or a keystroke captured by GLFW, as a simple hack.)

* Use openvdb::MeshToVolume to construct the distance grid.
* To get the grid into the GPU, I need to convert it to a 3D texture.
  See mTec, ``µTec::AssetManager::ReadSDFFromFile``.
* In Curv, ``file foo.stl`` returns a shape,
  which contains a distance function and a bounding box.
* When the distance function is compiled to GLSL, it is converted to
  an expression which references a 3D texture id.

So, right now, I use glslViewer. Each time the Curv file is recompiled,
I convert it into files (currently just a single fragment shader),
then glslViewer sees that the files have changed, and reloads them.

This complicates things. I don't think glslViewer can load a 3D texture.
If it could, how does it know that a new 3D texture has been added to the
model, during live editing?

New strategy. Convert glslViewer into library code that is linked into ``curv``.
When a Curv program is compiled, the fragment shader and any voxel grids are
constructed in memory, without writing them to files. Then OpenGL apis are used
to stuff these new assets into the GPU.
