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

My research into meshing algorithms suggests that producing a defect free mesh
in the general case ranges from difficult to impossible. But there are ways out.
* Mesh repair tools report an error for an STL file containing two cubes
  touching at an edge. This is a proper polyhedron with a non-manifold surface.
  STL is the only well known mesh format with no topology information.
  No error is reported for the same model in other file formats.
  Prusa Slicer deals with this model with no problem. (OpenSCAD could be
  changed to accept the 2-cubes model if stored in a mesh with topology.)
* The 3MF mesh format explicitly permits self-intersections. Slicers that
  support 3MF should handle this. Mesh repair programs will still report a
  problem.
* libigl provides fast, robust mesh booleans that deal with both of the above
  issues.
* OpenSCAD is very sensitive to non-manifold and self-intersecting meshes.
  The problem would go away if OpenSCAD were upgraded to use libigl.
* Or, Curv could add a mesh library based on libigl, which would fix the
  problem for me and other Curv users, since I wouldn't need OpenSCAD any more.

We can accept some mesh defects, in most situations:
* Non-manifold is okay, but only for a proper polyhedron. Otherwise,
  it won't pass 3MF validation, breaks libigl booleans, and is unacceptable.
* Self-intersection will pass 3MF validation and will work with libigl.
  Self-intersection is worse than non-manifold proper polyhedron, since
  mesh tools will report errors.

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
  https://github.com/aewallin/dualcontouring
  The output may contain non-manifold features. (But they are the safe kind?)
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

Manifold and Non-Intersecting
-----------------------------
The following is crap, do not use:

Watertight and 2-manifold Surface Meshes Using Dual Contouring with Tetrahedral Decomposition of Grid Cubes

2-manifold Surface Meshing Using Dual Contouring with Tetrahedral Decomposition

This 2016 paper claims to solve both problems. They claim the algorithm is
simple. Limitations: Can't do adaptive meshing. Although the mesh is
manifold, non-intersecting, and has high quality (non-sliver) triangles,
it looks like crap. Very stairsteppy where it should be smooth.
The suggestion is to perform a postprocessing step to smooth the mesh
(but this introduces defects).

Ugh, tradeoffs. Defect free == looks like crap.

https://www.sciencedirect.com/science/article/pii/S1877705816333422

2017, T. Rashid
"Multi-Material Mesh Representation of Anatomical Structures for Deep Brain Stimulation Planning"

Another version of the algorithm, this time supports multi-material.

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

TMC: Topologically Correct and Manifold...
==========================================
Variants of Marching Cubes and Dual Marching Cubes

Intended for meshing medical data at high resolution.
Produces bad results for sharp edges (and the result of meshing my tetrahedron
is not even "topologically correct" at the vertices).
Seems nice for meshing smooth models (no edges or corners).

Topologically correct means "the extracted triangle mesh is homeomorphic to
the level set of the underlying interpolant". (The input is a SDF inferred
from a voxel grid using trilinear interpolation, hence "interpolant".)
Seems to make the result a more accurate match to the model in some edge cases.
Can reproduce a tunnel through a grid cell.

"Manifold" had already been achieved for MC and DMC, but he uses a new
algorithmic technique that doesn't rely on a large, handcrafted lookup table.

"Parallel Reconstruction of Quad Only Meshes from Volume Data"
Jan 2020, 'awarded best paper of conference'
A improved version of Dual Marching Cubes that always produces topologically
correct output (unlike original, they say).
Watertight, manifold.
Fast CUDA implementation on github.
https://github.com/rogrosso/tmc

From the author Roberto Grosso:
  There is a misunderstanding when talking about Dual Marching Cubes.
  There are two different techniques to compute iso-surfaces called Dual MC:

  1. Dual Marching Cubes, Schaefer S. and Warren J., 2004. They compute first an
     octree and extract an iso-surface from the dual of the octree. Connecting
     neighbors to build triangles might result in non-manifold triangle mesh,
     i.e. holes or cracks.

  2. Dual MC, Nielson 2004. The intersection of an iso-surface with the faces
     of a voxel can be represented with a polygon, i.e. the Iso-surface can
     be considered to be a collection of polygons. The Dual MC we implemented
     computes the dual of this polygonal mesh. The result is a quad only
     mesh. It uses the full resolution of the input data, therefore you will
     get edges and thin structures. We use the asymptotic decider to resolve
     ambiguities, thus, the surface is watertight. There are some few cases,
     where the reconstructed surface is not homeomorphic to the iso-surface
     as defined in terms of underlying trilinear interpolant of the input
     volume grid. These are tunnel structures within voxels or between two
     neighbor voxels that happens in data sets which are geometrically very
     complex such as CT  data in medical applications. These tunnels can
     not be reconstructed with the method.

  Our method produces manifold meshes. Meshes are “topologically consistent”
  across cell borders: there are no holes, cracks or self-intersecting
  elements. It is not always “topologically correct” in the sense that
  there are cases, where the mesh is not homeomorphic to the iso-surface
  corresponding to the trilinear interpolant used to reconstruct the scalar
  function from a volume mesh. Tunnels within a voxel or across two neighbor
  voxels cannot be reconstructed with this method (certainly not either with
  the octree based method).

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

SHREC
-----
SHREC, based on a 2013 algorithm called MergeSharp, fixes bugs in Dual Contouring. This is university research; one author, Wenger, has published a book called "Isosurfaces" that deals with this stuff.

"Constructing Isosurfaces with Sharp Edges and Corners using Cube Merging" 2013
http://web.cse.ohio-state.edu/~wenger.4/papers/sharpiso.pdf

"Experimental Results on MergeSharp" 2013
ftp://ftp.cse.ohio-state.edu/pub/tech-report/2013/TR05.pdf

"SHREC: SHarp REConstruction of isosurfaces" 2015
ftp://ftp.cse.ohio-state.edu/pub/tech-report/2015/TR22.pdf

https://github.com/rafewenger/sharpiso
SHREC is a subproject. C++, LGPL, 982 commits over 3.7 years

The SHREC paper makes the following claim about the Simplicial... algorithm: Unfortunately, these algorithms create “notches” along sharp edges, degenerate, zero area triangles or quadrilaterals, and “folds” in the mesh with “flipped” triangles. (See Figure 1 for illustrations of these problems.)

The MergeSharp paper claims to fix the problems of degenerate triangles and self intersections caused by Dual Contouring, and in addition, is tolerant of noise in the distance function. This claim of robustness in the presence of noise is something I haven't seen before. There is one weakness: in the 2013 paper, the output is not guaranteed to be manifold. However, there is 2 years worth of additional work in the SHREC code, visible in git commits, which appears to address this problem.

The SHREC paper is a little more honest. "While MergeSharp reduced the mesh problems, it did not eliminate them, with many meshes still having one or two locations with degenerate mesh polygons or folds in the mesh." And: "SHREC, which almost completely eliminates the mesh problems listed above." And: "SHREC produces far fewer polygons with normal errors than any other software we tested, but it still occasionally produces such errors."

The implication of the SHREC paper is that nobody has a perfect SDF meshing algorithm that also detects sharp edges. The fewer errors you produce, the slower the algorithm is. (SHREC is slow.)

Matt Keeter says: (Jan 2019)
============================
Greetings from the meshing tarpit! I’ve implemented a bunch of different
algorithms over the years, and have been meaning to write them up in a single
place; this email is a start.

Doug listed a bunch of different properties; the ones that I group by are:

- Watertight
- Manifold
- Not self-intersecting
- Hierarchical (i.e. meshing flat planes with fewer triangles)
- Correctly reproducing sharp features (corners / edges)
- Correctly reproducing thin features

Marching cubes: not self-intersecting, no other desirable properties
Has issues with manifold / watertight / hierarchy, and edges are bevelled

Marching tetrahedrons: watertight, manifold(?), not self-intersecting
Is manifold / watertight, but non-hierarchical and edges remain bevelled.
There’s also a stronger non-isotropic behavior, based on how you build the tets

Cubical Marching Squares: Weird.
I agree with Doug’s assessment of this one.  For a while, I had the main implementation
on the internet (in kokopelli <https://github.com/mkeeter/kokopelli/blob/master/libfab/asdf/cms.c>); but it only implemented about 50% of the paper.
George Rassovsky <https://grassovsky.wordpress.com/2014/09/09/cubical-marching-squares-implementation/> wrote a different implementation after we corresponded, but I haven’t
looked into it – this algorithm is best left alone.

Feature Sensitive Surface Extraction from Volume Data: sharp features
This paper describes a way to detect triangles that are likely to contain an edge + corner,
and bump out an extra vertex to improve sharp feature performance.  It’s got a good explanation
of QEFs and how they can be built from position + normal samples and used to position vertices.

Dual contouring: watertight(?), hierarchical, sharp features
This is a strong local maxima in meshing algorithms – it’s not too hard to implement,
and works really well.  The main limitation is that meshes have self-intersections
and can be non-manifold.  There’s a good companion paper called “Dual Contouring:
The Secret Sauce” which talks about implementation details.

Dual Marching Cubes (Nielsen): Weird, and I don’t remember much about it,
but it apparently inspired Manifold Dual Contouring

Manifold dual contouring: manifold, watertight, hierarchical, sharp features
This is a worthwhile minor improvement to Dual Contouring, and the primary algorithm in libfive
It allows more than one vertex in a cell, to avoid non-manifold cases.  The main limitation
remains self-intersections.

Intersection-free Contouring on An Octree Grid: watertight, hierarchical, sharp features, not self-intersecting (probably)
This is an alternate improvement to Dual contouring, which tries to address the self-intersection issue.
It’s okay, but not great – it doesn’t handle the case where vertices completely escape their
containing octree cell.  It’s also not compatible with manifold dual contouring: if you try to
combine the two, then you can end up with self-intersections in a multi-vertex cell.

Dual Marching Cubes (Warren): manifold, watertight, hierarchical, sharp features, thin features
There are clever ideas to internalize, but I’ve found obvious cases where it just doesn’t work;
I’m not sure if I’m misunderstanding something or the algorithm just isn’t that great.  On the other
hand, the idea of positioning vertices on sharp features of the underlying field led to….

Isosurfaces Over Simplicial Partitions of Multiresolution Grids: watertight, manifold, not self-intersecting, hierarchical, sharp features, thin features (everything!)
This is what I’m trying to implement in libfive right now.  It’s a very clever algorithm that has
all of the desirable properties, and builds on a lot of other ideas (Dual Contouring, Warren’s DMC
and Marching Tetrahedrons).  It’s not too much more complicated than dual contouring in theory, but
you have to be really comfortable with QEFs – I ended up writing a long explainer <http://www.mattkeeter.com/projects/qef/> while digging
through the math.  It’s much harder to make fast, and produces many more triangles unless you implement
some of the bonus algorithms from the paper.  Also, the paper only describes top-down construction,
which I’m skeptical about, so I’ve developed a bottom-up way to build the octree (to guarantee finding
shapes above a certain size, rather than trusting a heuristic + sampling).

Solid Meshing
=============
The other methods explored here partition 3D space into a cubic grid and use
a marching-cubes inspired method to mesh an isosurface. With Solid Meshing,
the volume enclosed by the isosurface is meshed to polyhedra, and a surface mesh
is generated as a side effect. Much slower. The quality is potentially better.

In theory, by partitioning a solid into polyhedra, the surface mesh is
guaranteed by construction to be watertight and non-self-intersecting.
We seemingly have no such guarantees for isosurface meshing algorithms that
also perform sharp feature recognition (some papers claim this, but the testing
reported in the SHREC paper debunks such claims). [Wang 2016] also claims that
output is stable across rigid transformations, which is desirable.

Solid meshing algorithms are intended to provide solid meshes that are useful
for engineering purposes, such as finite element analysis. I don't care about
the interior mesh, so maybe we can derive a faster algorithm by minimizing the
number of internal polyhedra, subdividing to small polyhedra only near the
surface.

Vorocrust
---------
Not open source, algorithm is patented. At first glance the output looks
quite nice and it seems to meet all requirements. But, it sacrifices accuracy
of the surface mesh to improve the interior mesh, which is not good for
"implicit forms". (Although the loss of accuracy is controlled by tolerance
parameters.)

* VoroCrust is the first correct algorithm for conforming Voronoi meshing of
  non-convex and non-manifold domains with guarantees on the quality of both
  surface and volume elements.
* The most recent variation of the algorithm generates an automatic sizing
  functions and can handle sharp features and non-manifold domain.
* https://arxiv.org/abs/1902.08767
* Does the following make Vorocrust unsuitable?
  * VoroCrust always retains the topology of the domain but is not restricted
    to conform exactly to the boundary; it effectively performs remeshing to
    improve the output quality within the tolerance specified by the input
    parameters.
  * Another limitation is the requirement that the input triangulation is a
    faithful approximation of the domain. This inhibits the application of this
    approach to implicit forms [Wang et al.2016]. See below.

"On Volumetric Shape Reconstruction from Implicit Forms" [Wang 2016]
--------------------------------------------------------------------
 * Li Wang, Franck Hetroy-Wheeler, and Edmond Boyer. 2016.  On volumetric shape reconstruction from implicit forms. In Computer Vision– ECCV 2016. 173–188.
 * https://hal.inria.fr/hal-01349059/document

It claims higher quality output than marching cubes:

 * manifold, watertight, no self intersection, by construction.
 * Output is stable across rigid transformations.

Unlike Vorocrust, automatic sharp feature recognition is not supported.
40-100x slower than MC.

Existing approaches for 3D implicit form conversions into volumetric
tessellations can be roughly divided into two categories.

 * A first category, that includes Marching Cubes and its extensions, partitions
   the observation domain Ω into cells on a fixed grid (usually cubic).
   Inside and outside cells are identified with respect to the input implicit
   function and the boundary cells can be further polygonized into a triangle
   mesh approximating the shape surface. This strategy is efficient and fast
   and is widely used. However the 3D shape discretization into cubic cells
   with constrained orientations produces a poor shape tessellation which can
   result in surface approximations with elongated or small triangles. Thus,
   an additional re-meshing step is consequently often required. In addition,
   attaching the grid to Ω makes the tessellation change with any shape
   transformation, even rigid.
 * A second category, such as [8] with Delaunay tetrahedrization, discretizes
   instead the inside region V. This usually gives better shape tessellations
   which are as well independent of rigid shape transformations and hence
   plausibly better suited for dynamic scene modeling. Still, they require
   expert control to monitor the cell refinement step that is performed.
   Moreover, as the boundary of a tetrahedral structure can present non manifold
   parts it is difficult to guarantee a correct topology for the boundary mesh
   approximating the surface. In this paper, we explore a different strategy
   that also belongs to the second category and discretizes V instead of Ω.
   The approach builds on Centroidal Voronoi Tessellations (CVTs) that provide
   regular shape tessellations which boundaries are obtained by clipping
   frontier cells with the given implicit boundary form. In contrast to Delaunay
   based methods, the boundary surface of the output volume is, by construction,
   manifold and the approach has only a few parameters.

Mesh Optimization
=================
These algorithms begin with an initial mesh created by one of the grid-based
meshing algorithms. Then, they iteratively improve it using an "energy
minimization" strategy, with high quality results, better looking than
grid-based output. They are slow, but there is recent research on making them
performant.

Ideally, I want a remeshing algorithm that uses the original distance function
to measure the accuracy of new vertices, and which does sharp feature detection,
and which produces defect free output.

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

"**Locally-optimal Delaunay-refinement and optimisation-based mesh generation**".
https://github.com/dengwirda/jigsaw
Looks interesting, but has a non-commercial, non-free licence.

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
