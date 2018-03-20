A Hybrid V-Rep Data Structure
=============================
There are many V-Rep representations with different strengths and weaknesses.
Curv will use a hybrid representation, so that it can more efficiently represent
a larger set of shape constructors and a larger set of shape operations, and so
that it scales to efficiently render large scenes containing many elements.

At the top level is a "ray tracing acceleration structure", probably a BVH
(bounding volume hierarchy). This tree speeds up ray tracing by restricting
the set of top level shapes that each ray needs to be intersected against.
Each leaf in this tree can use a different volume representation.

The design of the acceleration structure is considered critical for high end
ray tracers, and a ton of research has been done on optimizing these structures
for the GPU.

At the leaves, I plan to support:

* Sphere traced implicit functions.
* Ray traced implicit functions using interval arithmetic: slower than
  sphere tracing, but supports a larger set of implicit functions.
* Volume data structures, various kinds of discrete signed distance fields.
  Eg, voxel arrays are the most popular on the GPU.
  Eg, efficient and/or accurate volume data structures for representing meshes.
