Volume Data Structures
======================
A "Volume Data Structure" is a volumetric representation of a geometric shape,
as distinct from a volumetric function representation.

Volume data structures expected to be more efficient than functions
in a number of cases:

* For large unions, it's more efficient to traverse a bounding volume
  hierarchy than to evaluate all of the leaves and min all the distances together.
* For converting a triangle mesh into a signed distance field representation.
* For sweeping a 3D shape along a 3D path.
* For algorithms that iteratively generate a long sequence of points that
  define the shape (with no obvious implicit function representation),
  we can accumulate data in a volume data structure.

How many distinct volume data structures do we need to cover the important
use cases?

On the GPU, voxel arrays hardware accelerated, and are therefore an efficient and
popular choice for representing a discrete signed distance field.

VDB is a popular hierarchical data structure built from a tree of voxel arrays.
The OpenVDB project is well supported and widely used (for CPUs, with an OpenGL renderer).

GVDB Voxels (2017, BSD licence) is an nVidia CUDA implementation of VDB.

Converting Meshes to Signed Distance Fields
-------------------------------------------
There are two kinds of representations to consider:

* An *exact* representation of a mesh preserves all of the faces and edges.
  This is appropriate for smaller triangle meshes that are intended to
  represent polyhedra.

* An *approximate* representation will smooth over regions that represent
  curved surfaces, while preserving sharp features. In theory, this would be
  a more memory efficient way to represent large triangle meshes that are approximations
  of curved surfaces.

The most promising exact representation is the bounding volume hierarchy used
to ray trace large triangle meshes.

A voxel array or VDB is a popular approximate representation.
However, there is no sharp feature detection.

"Efficient Sparse Voxel Octrees"
http://research.nvidia.com/sites/default/files/pubs/2010-02_Efficient-Sparse-Voxel/laine2010i3d_paper.pdf

It's from nVidia, and is intended for use in video games. Each node of the octree has both voxel data, and a "contour" which provides boundary information. The contours allow it to "approximate sharp corners". They converted meshes to this data structure in order to test it. (Similar to the concept of VDB. But, note, the "contour" mechanism was not absorbed by the newer "GDB Voxels" product.)
