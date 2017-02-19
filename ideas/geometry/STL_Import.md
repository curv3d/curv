# Importing STL Files into Curv

This seems like a difficult problem. ImplicitCAD and Antimony can't do it.

How do you convert a large mesh into an efficient F-Rep representation?

Some ideas:
 1. Don't try. Instead, build a hybrid geometry system where some shapes are
    meshes, and some shapes are F-Rep.
    * How do you render a mixed scene?
      * Start with: top level union of mesh and f-rep using GPU.
        Eg, render mesh using zbuffer, then render f-rep using ray-marching
        and zbuffer.
        http://stackoverflow.com/questions/23362076/opengl-how-to-access-depth-buffer-values-or-gl-fragcoord-z-vs-rendering-d
        https://www.khronos.org/opengl/wiki/Image_Load_Store
      * Of the geometry operators, some work only on mesh (eg convex hull),
        some work only on f-rep, some work on either (but not both at once).
      * Some kind of unknown GPU magic. Maybe render a mesh to an offscreen
        Z-buffer then ???
      * Converting mesh to F-Rep is the problem we are trying to avoid.
      * Converting F-Rep to mesh is a solved problem.
    * I think maybe IceSL is a hybrid system. Try to figure out what they do.
 2. Convert the mesh offline into a more efficient volumetric representation.
    * An ASDF octree representation.
    * "Efficient Sparse Voxel Octrees". Figure out how this system works.
      https://mediatech.aalto.fi/~samuli/publications/laine2010tr1_paper.pdf
    * "Euclideon Infinite Details" is a proprietary voxel system that
      allegedly keeps a large voxel database on disk then efficiently loads
      just the data needed right now.
      https://en.wikipedia.org/wiki/Euclideon
    * The key is data compression. An F-Rep system can efficiently represent
      a very complex scene in a small number of bytes using procedural
      representation. So, find a way to compress a large mesh (like Yoda,
      600K triangles) into a much smaller procedural representation.
      Patterns are recognized and encoded, underlying symmetries are exploited,
      unwanted noise is smoothed away and simplified.
      * Since pattern recognition is involved, maybe use deep learning /
        neural network techniques.
 3. In the papers I've read on mesh->f-Rep conversion, constructing the
    initial data structure is too slow, plus the resulting data structure can
    be huge. Hence the suggestions above for offline conversion and compression.
    * one paper created a BSP
    * one paper created an octree
