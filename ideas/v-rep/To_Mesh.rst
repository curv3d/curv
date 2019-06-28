========================================
Converting a Distance Function to a Mesh
========================================

Ideally, there would be a standard, popular library, prepackaged by Ubuntu,
that I could just link to and use. In this category, the closest I've found
so far are:

* libigl -- has a GPL'ed marching cubes
* vcglib (the library underlying meshlab) has a GPL'ed marching cubes
* openvdb -- has a marching cubes that outputs quads and triangles,
  with no quality problems.

Ideally, the meshing algorithm will create a mesh that looks good (is a good
approximation of the original model), can be successfully used in OpenSCAD (no
CGAL errors), and can be successfully 3D printed. And the algorithm is fast.

Mesh Quality
============
For 3D printing, and especially for import to OpenSCAD, you want a defect free
mesh. This means: watertight, 2-manifold, with no self-intersections,
degenerate triangles, or flipped triangles*. 3D printing tools have the ability
to repair at least some problems, but OpenSCAD has zero tolerance for defects.

[*] 2-manifold implies no flipped triangles, but the first 4 properties are
orthogonal. The most general definition of 2-manifold does not imply
watertightness, but some authors restrict 2-manifold meshes to also be
watertight. However, the 2-manifold property does not exclude self-intersection
under any definition. Some people erroneously use 'manifold' as a synonym for
defect-free.

For 3D printing and OpenSCAD, you also want to minimize the number of triangles.
Past a certain limit, STL files become impossible to process, and as you
approach that limit, processing becomes unacceptably slow on a non-linear scale.
This means you want adaptive mesh generation: triangle size decreases as
local curvature increases. Triangles are as large as possible, consistent with
approximating the surface within a specified error tolerance.

Modern implementations of marching cubes produce defect-free meshes.
However, the algorithm is not adaptive: triangle sizes are uniform.
I haven't found an open source algorithm for simplifying a mesh that doesn't
also introduce defects. Meshlab's Quadric Edge Collapse Decimation introduces
self-intersections.

Marching cubes also does not detect sharp features (vertices and edges).
Instead, these features get rounded over. Unfortunately, algorithms that
do perform edge detection (beginning with Dual Contouring) also produce
defective meshes. The most common problem is self-intersection.

This problem with edge detection and self-intersection is, in part, a result
of working with floating point numbers. The CGAL project provides a good
explanation for why this is: https://www.cgal.org/exact.html

    The second and deeper problem is that numerical weapons are per se
    less effective in geometric computing than they are in other fields.
    In geometry, we don't compute numbers but structures: convex hulls,
    triangulations, etc. In building these structures, the underlying
    algorithms ask questions like "is a point to the left, to the right,
    or on the line through two other points?" Such questions have no
    answers that are "slightly off". Either you get it right, or you
    don't. And if you don't, the algorithm might go completely astray.
    Even the most fancy roundoff control techniques don't help here:
    it's primarily a combinatorial problem, not a numerical one.

Nobody has cracked the problem of writing a meshing algorith that does sharp
feature detection and which also generates defect-free meshes, while using
floating point (approximate) computation. The problem might well be impossible
to solve. The SHREC algorithm appears to do the best job, so far, but even
SHREC doesn't guarantee defect free output in all cases. But I haven't tested
SHREC yet.

Libfive provides three meshing algorithms. All are manifold and watertight,
but they have other quality issues.
https://github.com/libfive/libfive/issues/284
* Manifold Dual Contouring: -> self intersections
* Matt Keeter says: ISO and Hybrid both have wrinkles and issues preserving
  sharp features. Both should be free of self-intersections, though may have
  zero-area triangles and other questionable geometry (e.g. zero-volume cracks)
  to keep the triangle graph manifold.
  Michael Hoffer says ISO has self intersection.

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
"Dual Marching Cubes: Primal Contouring of Dual Grids"
(2004) Scott Schaefer and Joe Warren
https://www.cs.rice.edu/~jwarren/papers/dmc.pdf

Benefits:

* Crackfree, adaptive, reproduces sharp features.
* Reproduce thin features without excessive subdivision
  required by Marching Cubes or Dual Contouring.
  (Fewer voxels than MC to reproduce thin walls!)
* Able to conform to the relevant features of the
  implicit function yielding much sparser polygonalizations
  than (other grid methods). (Fewer triangles!)
* Topologically manifold!
* The intermediate data structure might be generally useful:
  it is "a piecewise linear approximation to
  a given function over a cubical domain".
* Needle shaped triangles: has a parameter to avoid generating
  triangles that match a skinnyness threshhold.

Drawbacks:

* Intersecting triangles.

Questions:

* watertight?

"Manifold Contouring of an Adaptively Sampled Distance Field"
(2010) ELIAS HOLMLID
http://publications.lib.chalmers.se/records/fulltext/123811.pdf

A variant implementation of Dual Marching Cubes.

* well explained. provides a good explanation of DMC.
* simplicity of code was a goal
* The goal was to contour an ASDF (adaptively sampled distance field, stored in an octree).
  Dual marching cubes is (claimed to be) a good match to this distance field representation,
  since you don't need to compute and store Hermite data, you don't need direct access to
  the original implicit function.
* avoids self intersection by restricting generated vertexes to lie inside voxels,
  actually, in the centre of the voxel. This has a negative effect on sharp feature detection,
  and eliminates the "thin feature reproduction" feature of DMC.
* ...lost interest

SPMG: Simplicial Partitions of Multiresolution Grids
====================================================
"Isosurfaces Over Simplicial Partitions of Multiresolution Grids"
(2010) Josiah Manson and Scott Schaefer
http://faculty.cs.tamu.edu/schaefer/research/iso_simplicial.pdf

Benefits:

* A function is known for all points in a bounded region.
  We only assume that the function is piecewise smooth and does not
  have to be a distance function.
* Efficient: adaptive sampling of the function, doesn't require evaluating
  the function at every voxel in a uniform grid.
* manifold
* intersection-free
* reconstructs sharp features
* reconstructs thin features beyond the sampling resolution of the octree
* adaptive: an error metric designed to guide octree expansion
  such that flat regions of the function are tiled with fewer polygons than
  curved regions to create an adaptive polygonalization of the isosurface.
* mesh optimization: We then show how to improve the quality of the
  triangulation by moving dual vertices to the isosurface and provide a
  topological test that guarantees we maintain the topology of the surface. 

Drawbacks:

* skinny triangles.
* slower than DC and DMC.
  DMC is approximately 10-20% slower than DC, and our method takes an additional 50% longer than DMC.

This is the highest quality grid method I've seen so far (to create the initial
mesh, before optimization).

Source code? Manson was the student, Schaefer was the faculty advisor.
Here's Manson's code, with a non-commercial licence: http://josiahmanson.com/research/iso_simplicial/

Mesh Optimization
=================
These algorithms begin with an initial mesh created by one of the grid-based
meshing algorithms. Then, they iteratively improve it using an "energy minimization"
strategy, with high quality results, better looking than grid-based output.
They are slow, but there is recent research on making them performant.

"**Dual/Primal Mesh Optimization for Polygonized Implicit Surfaces**" (2002)
Yutaka Ohtake and Alexander G. Belyaev
http://www.hyperfun.org/SM02ob.pdf

Open source implementations:

* http://home.eps.hw.ac.uk/~ab226/software/mpu_implicits/webpage.html
  C++, by original authors. Warning: no copyright notice or copyright licence.
* https://github.com/sohale/implisolid
  C++ and Python, LGPL 3.
* https://github.com/Lin20/BinaryMeshFitting
  C++, MIT Licence. Looks worth investigating.
  It's a high performance implementation embedded in a game engine.
  Uses DMC to create the initial mesh (DC didn't work as well).
  Still under active development.

"**Locally-optimal Delaunay-refinement and optimisation-based mesh generation**".
https://github.com/dengwirda/jigsaw
Looks interesting, but has a non-commercial, non-open source licence (not open source).

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
