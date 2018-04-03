Volume Data Structures
======================
A "Volume Data Structure" is a volume representation of a geometric shape,
as distinct from a volume function representation.

Volume data structures expected to be more efficient than functions
in a number of cases:

* For large unions, it's more efficient to traverse a bounding volume
  hierarchy than to evaluate all of the leaves and min all the distances together.
* For converting a triangle mesh into a signed distance field representation.
* For sweeping a 3D shape along a 3D path.
* For algorithms that iteratively generate a long sequence of points that
  define the shape (with no obvious implicit function representation),
  we can accumulate data in a volume data structure.

It would be nice to find a universal volume data structure:

* Can be raycast in real time on a GPU.
* A mesh can be converted to the VDS.
* The VDS can be converted to a mesh.
* Supports boolean operations (union, intersection, difference)
  and sweeping a 3D shape along a 3D path. Basically, digital sculpting ops.
* Sharp features are preserved.

Practically, there are tradeoffs between memory, performance and generality.
So how many distinct volume data structures do we need to cover the important
use cases?

Since I'm reading a lot of research papers:
Has the method been successfully used by people other than the researchers?
Is it available as open source?

Requirements and Criteria
-------------------------
Some criteria for evaluating potential Volume Data Structures (VDSs).

Efficient Raycasting
  If the VDS is the top level shape in a scene, there is an efficient
  algorithm for raycasting the shape in real time on a GPU.

Global SDF
  The VDS has a Signed Distance Field (SDF) that is defined at all points in 3D space.

Continuous SDF
  The SDF is C0 continuous.
  This might be an issue if the VDS is a hierarchical assembly of "patches"
  which specify different regions of the SDF -- we don't want discontinuities where
  patches are joined together.

Lipschitz(1) SDF
  Ideally, the SDF is Lipschitz(1), so that if L(1)-preserving shape operations are applied
  to the VDS field, the resulting SDF can be sphere traced.
  (This is stronger than C0 continuity.)

Euclidean SDF
  Ideally, you can construct an SDF that approximates a desired Euclidean SDF
  to within an error tolerance.
  Applications include offsets and blending.
  (I intend this to be stronger than L(1) continuity -- approximation errors in the
  approximate Euclidean SDF don't violate the L(1) condition.)

Conversion from Implicit Function
  It should be possible to convert an arbitrary implicit function into a VDS that
  approximates it, within a specified error tolerance.
  One application is offline conversion of an implicit function that is too expensive
  for interactive raycasting to a cheaper VDS that can be raycast.
  Also of interest is repairing
  an implicit function to make its SDF Lipschitz(1) or Euclidean.

Conversion from Triangle Mesh
  Two interesting cases for conversion from a triangle mesh.
  First is exact conversion, where the result is a polyhedron.
  Second is to create an approximation to within an error tolerance.

Conversion to Triangle Mesh
  ...

Efficient representation of flat surfaces, curved surfaces, sharp features
  ...

Efficient representation of thin features
  ...

Animation
  Can animation be efficiently supported?
  For example, we could extend the data structure to describe a 4D distance field
  that maps (x,y,z,t) to a distance.

Varieties of Volume Data Structures
-----------------------------------
3D arrays of distance values (**voxel arrays**) are a universal volume data structure.
On the GPU, voxel arrays are hardware accelerated. This all makes them a
popular choice for representing a discrete signed distance field. Two problems:

* They can consume a lot of memory.
* They don't reproduce details finer than the grid resolution.
  Sharp features are rounded off (due to trilinear interpolation).

Most alternatives to voxel arrays are tree structured.

**VDB** is a popular hierarchical data structure built from a tree of voxel arrays
(they are called sparse voxel arrays).
The OpenVDB project is well supported and widely used (for CPUs, with an OpenGL renderer).
GVDB Voxels (2017, BSD licence) is the 2nd generation nVidia CUDA implementation of VDB.
Reproducing sharp features is a problem.

Marching Cubes works directly on a grid of distance values. Sharp features are not preserved.

Dual Contouring (2002) uses an octree that tracks where the surface intersects grid cell edges,
and stores "hermite data" (exact intersection points and their normals), the latter used to reproduce
sharp features. This is not a representation of a signed distance field.

Extended Marching Cubes (2001, "Feature Sensitive Surface Extraction from Volume Data")
uses an "enhanced SDF representation" that preserves information about sharp features.
It's an octree representation of a directed distance field. Interesting.

Dual Marching Cubes (2004) uses an octree of distance values, where the recursive subdivision
of the octree is guided by Quadratic Error Functions. Compared to Dual Contouring and Extended
Marching Cubes, a "much sparser" octree is required. From the octree, a "dual grid" is constructed,

Converting Meshes to Signed Distance Fields
-------------------------------------------
There are two kinds of representations to consider:

* An *exact* representation of a mesh preserves all of the faces and edges.
  This is appropriate for smaller triangle meshes that are intended to
  represent polyhedra.

* An *approximate* representation will smooth over regions that represent
  curved surfaces (ideally while preserving sharp features). In theory, this would be
  a more memory efficient way to represent large triangle meshes that are approximations
  of curved surfaces, where you can trade off memory for precision.

For each conversion method, we should consider:

* What is the representation?
* Does the method require a valid mesh (manifold or watertight, and non-self-intersecting)?
  Or does it work on triangle soup?

Signed Distance Fields for Polygon Soup Meshes
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
"Signed Distance Fields for Polygon Soup Meshes" (2014) http://run.usc.edu/signedDistanceField/

* Works for polygon soup.
* Easy to control, with a single parameter that determines the size of the holes that will be filled in.
* Doesn't support internal voids -- these will be filled in.
* Output is a discrete SDF, a grid of distance values. The distance values may be exact.

Exact Mesh Representation
~~~~~~~~~~~~~~~~~~~~~~~~~
The bounding volume hierarchy (BVH) used to accelerate ray tracing of a mesh
is a promising starting point for an exact mesh representation.
This structure is pretty much optimal for ray-tracing a mesh based shape.
It won't give you an exact distance to the nearest triangle in constant time,
however.

https://www.researchgate.net/publication/262215434_Efficient_evaluation_of_continuous_signed_distance_to_a_polygonal_mesh

This looks like a great paper. Lots of performance testing, including CPU vs GPU implementations. They use bounding volume hierarchies, similar to what's used for ray tracing. Requires a valid mesh.

The GPU performance is roughly comparable to CPU performance (with 12 cores) for the biggest models with the most triangles, Armadillo and Buste. In those models, large numbers of triangles are used to approximate curved surfaces: for these particular models, an exact distance field is not valuable. The GPU is 37 times faster than the CPU for the smallest model, "signbreaker", a polyhedron that needs to be represented exactly.

It would be nice to compare performance of this algorithm to voxel arrays (which give approximate distance fields) for different size meshes.

There is a follow-up by the same authors, where they run into problems with the exact distance fields created by this technique. They discuss the use of convolution to modify the distance field and fix the problems they encountered:

http://eprints.bournemouth.ac.uk/22532/1/SFFP15_FilteringSDF_CGF.pdf

Approximate Mesh Representation
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
A voxel array or VDB is a popular approximate representation.
However, there is no sharp feature detection.

"Efficient Sparse Voxel Octrees"
http://research.nvidia.com/sites/default/files/pubs/2010-02_Efficient-Sparse-Voxel/laine2010i3d_paper.pdf

It's from nVidia, and is intended for use in video games. Each node of the octree has both voxel data, and a "contour" which provides boundary information. The contours allow it to "approximate sharp corners". They converted meshes to this data structure in order to test it. (Similar to the concept of VDB. But, note, the "contour" mechanism was not absorbed by the newer "GVDB Voxels" product.)
