========================================
Converting a Distance Function to a Mesh
========================================

Ideally, there would be a standard, popular library, prepackaged by Ubuntu,
that I could just link to and use. In this category, the closest I've found
so far are:

* libigl -- has a GPL'ed marching cubes
* vcglib (the library underlying meshlab) also has a GPL'ed marching cubes

Ideally, the meshing algorithm will create a mesh that looks good (is a good
approximation of the original model), can be successfully used in OpenSCAD (no
CGAL errors), and can be successfully 3D printed. And the algorithm is fast.

Marching Cubes
==============
Marching Cubes (1987) works by subdividing space into a voxel grid, and sampling
the distance function at each voxel corner. Between 0 and 4 triangles are
generated inside each voxel, with vertices on the voxel edges. If the grid is
fine enough to capture all the details in your model, then you may end up with a
lot more triangles than you need. So you may want to post-process using a mesh
simplifier.

Marching Cubes works great on spheres. It works poorly on cubes -- the edges and
corners are rounded off. A careful, modern implementation creates a watertight,
manifold mesh. Marching Cubes doesn't meet my goals because it doesn't render
cubes properly.

Dual Contouring
===============
Dual Contouring (2002) is another popular grid-based method. It detects features
(sharp edges and corners), so cubes are rendered correctly. If properly
implemented, I think the mesh is watertight, but it produces self-intersections,
and the mesh may not be manifold. The code is reasonably simple. There are
multiple implementations on github, although they all appear to be one-person
hobby-grade implementations. There are multiple GPU implementations.
The algorithm is well understood, and even has its own subreddit.

Does Dual Contouring meet my goals?

1. The results look good.
2. It produces self intersections, which cause OpenSCAD booleans to fail.
3. It can create non-manifold meshes, but of a specific type that are
   compatible with OpenSCAD CGAL. The mesh is watertight, but two separate
   objects can share a vertex or edge, or two projections of the same object
   can meet at a vertex or edge. As far as I know, these objects are 3D
   printable. So, no problem.
4. It potentially creates lots of triangles, which could cause you to run out
   of time or memory while performing CGAL operations in OpenSCAD. Ideally,
   there are exactly enough triangles to approximate the surface, up to a
   specified error tolerance. This means: more and smaller triangles in regions
   of high curvature, and fewer and larger triangles in regions of low
   curvature.
5. It can produce needle shaped triangles, which might be an issue?
6. Multiple GPU implementations (and research papers), which will help
   to speed things up later.

Issues #2 and #4 above are the main issues that need to be addressed.

Self intersection
-----------------
* @emilk on github implemented a simple "clamping" method to eliminate self
  intersections, but this produces artifacts that look bad. Clamping also
  helps with needle shaped triangles.
* “Intersection-free contouring on an octree grid” (2006) eliminates the
  self-intersections by devising a set of simple geometric tests to identify
  potentially intersecting polygons, which are then tessellated into smaller,
  non-intersecting triangles.
  Source (LGPL2.1+) https://sourceforge.net/projects/dualcontouring/
* LibIGL has an algorithm (GPL3) to repair meshes with self intersection.
  ``include/igl/copyleft/cgal/remesh_self_intersections.h``
* "Direct repair of self-intersecting meshes"

Simplification
--------------
Once I have a mesh with no self intersections, I'd like to simplify it / remesh
it so that there are fewer triangles. I'd like to control this using an error
tolerance. As an added bonus, maybe use the original distance field as a guide
to bound the approximation errors. Maybe this algorithm also fixes
needle shaped triangles.

Manifold Mesh
-------------
As mentioned above, I don't think the non-manifold output of dual contouring
is a problem for me. However, another user of Curv might have different
requirements, or my requirements could change.

"Manifold Dual Contouring" (2007) produces a manifold mesh, but there are still
self intersections (which will cause OpenSCAD booleans to fail). There's
signficant added complexity to fix this problem.  @Lin20 has implemented
manifold dual contouring on github (in C#); he claims it's the only public
implementation.

Dual Marching Cubes
===================

Dual/Primal Mesh Optimization
=============================
This algorithm begins with a "primal mesh" created by one of the grid-based
meshing algorithms. Then, it iteratively improves it using an "energy minimization"
strategy, with high quality results, better looking than grid-based output.

"Dual/Primal Mesh Optimization for Polygonized Implicit Surfaces" (2002)
Yutaka Ohtake and Alexander G. Belyaev
http://www.hyperfun.org/SM02ob.pdf

Open source implementations:

* http://home.eps.hw.ac.uk/~ab226/software/mpu_implicits/webpage.html
  C++, by original authors. Warning: no copyright notice or copyright licence.
* https://github.com/sohale/implisolid
  C++ and Python, LGPL 3.
* https://github.com/Lin20/BinaryMeshFitting
  C++, MIT Licence. Looks worth investigating.
  Still under active development.

Survey of Algorithms
====================
A survey of meshing algorithms:
http://webhome.cs.uvic.ca/~blob/publications/survey.pdf
"A Survey on Implicit Surface Polygonization", 2014

There are a huge number of available algorithms.
There's an engineering tradeoff between quality and speed.
Most of the algorithms described in research papers don't seem to be
available as open source.

Fast meshing algorithms descend from Marching Cubes: they divide space into
regularly sized tiles, usually cubes, sometimes tetrahedra.
Then sample the distance function and create triangles at each tile.
Features smaller than a tile may be lost.
I'm currently focused on Dual Contouring because it has many open source
implementations and is well understood.

High quality meshing algorithms create a high quality, adaptive mesh:

* No needle shaped triangles. Angles between 30 and 120 degrees.
  Eg, in 2D, use Delauney triangulation.
* Lots of small triangles in areas of high curvature. Fewer larger triangles
  in areas of low curvature.
* Features (edges and corners) are preserved.

They typically use an expensive, iterative algorithm (eg, energy minimization).

Remeshing the output of a grid based algorithm like dual contouring
is one way to get higher quality output.
