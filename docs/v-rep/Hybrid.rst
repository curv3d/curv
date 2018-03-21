A Hybrid V-Rep Data Structure
=============================
Curv will use a hybrid V-Rep representation. It's a tree.
Each leaf in this tree can use a different volume representation.

This design supports two goals:

* The tree will speed up ray tracing of large scenes containing many elements.
* There are many V-Rep representations with different strengths and weaknesses.
  By supporting multiple V-Reps at the leaves, Curv can more efficiently
  represent a larger set of shape primitives.

The Tree
--------
Compiling a large F-Rep CSG tree into a single monolithic GPU kernel
doesn't give good ray-tracing performance. Once the kernel gets too big,
you run out of registers and overwhelm the instruction cache. These latter
effects are worse than just the slowdown of evaluating all the shapes for
every ray: you run off a performance cliff once kernel size passes a threshold.

The first step (before I get to Hybrid) is an experiment: can I reorganize
the monolithic kernel code into a set of noninlined functions, so that I
don't run out of registers? Can I avoid the performance cliff and achieve
linear slowdown with the number of shapes unioned? That's a good exercise
to try, prior to the next step.

The next step is to optimize the ray-tracer so I don't evaluate every
primitive on every ray. E.g., partition the viewport into "beams", and use a
"culling" kernel to constrain the set of leaf shapes that each ray in the beam
must be intersected with. The output is an array of leaf ids. A subsequent
kernel ray-traces each beam (using sphere tracing or interval arithmetic).

* This requires the raytracing kernel to be a megakernel containing the
  code for all of the F-Rep leaves. I will need to compile this kernel in
  such a way that one F-Rep leaf can't steal registers from another F-Rep
  leaf, as if each leaf were in its own kernel. Basically, each F-Rep leaf
  goes into its own function, which cannot be inlined.
* Once the problem with register spilling is dealt with, the theory is that
  this megakernel will avoid calling functions that aren't in the array of
  leaf ids constructed by the culler. Thus, it runs faster and the instruction
  cache won't be thrashed. Without culling, you have to evaluate every leaf in
  the top level union, then min all the results together, which takes too much
  time and stresses the instruction cache.

The acceleration structures used by high end ray tracers speed up ray tracing
by constraining the set of top level shapes that each ray needs to be
intersected against. A bounding volume hierarchy (BVH) is a common choice.
This is a tree of axis aligned bounding boxes, with usually either 2 or 4
children under each interior node.

The design of the acceleration structure and the ray traversal algorithm
is considered critical for high end ray tracers, since you can have up to a
million nodes in the tree, and you want real-time performance.
A ton of research has been done on optimizing these structures
for the GPU. General purpose GPU ray-tracing frameworks are available from
Nvidia (OptiX and now RTX), AMD (ProRender),
and Microsoft (DXR, part of DirectX 12, with support from Nvidia and AMD).
Intel is missing: they are betting on CPU-based ray tracing instead, with
their Embree framework. DXR has a fallback for GPUs with no hardware raytracing
support, but Microsoft doesn't promise good performance. Vulkan will
need to respond with its own portable cross-platform ray-tracing API.

* "Understanding the Efficiency of Ray Traversal on GPUs"
  https://users.aalto.fi/~ailat1/publications/aila2009hpg_paper.pdf
  https://github.com/matt77hias/GPURayTraversal
* "OptiX: A General Purpose Ray Tracing Engine"
  http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.677.323&rep=rep1&type=pdf
* https://blogs.msdn.microsoft.com/directx/2018/03/19/announcing-microsoft-directx-raytracing/

Animation complicates things: in these frameworks, you somehow need to update
the acceleration structure when objects move. ???

In OptiX and DXR, the leaf nodes of the acceleration structure are 'primitives'
that can be represented and rendered any way you want. They can be sphere
traced F-Rep. DXR lets you assign a different shader to each primitive,
so apparently a megakernel isn't needed.

These acceleration structures are optimized for representing a union of a
large number of convex objects. While that is an important use case for Curv,
I may wish to explore a more general kind of acceleration structure.

In "Interval Arithmetic and Recursive Subdivision for
Implicit Functions and Constructive Solid Geometry", Tom Duff describes a
method for efficiently rasterizing a CSG tree containing a large number of
primitives. His tree supports both unions and intersections at the interior
nodes.

Curv also supports repetition operators, which allow you to fill space with
multiple, or an infinite number of, copies of some primitive, arranged in
space according to a mathematical pattern. Maybe the culling kernel needs
to be aware of these kind of repeated primitives. I.e., cull a repetition object
if the beam passes between instances of the repeated primitive, since maybe
a bounding box test is not enough.

A megakernel raytracer might not be the best design, even with culling.
An alternative is discussed here:
http://www.highperformancegraphics.org/wp-content/uploads/Laine-MegakernelsConsideredHarmful.pdf

This latter approach would, I think, let me partition my F-Reps across
multiple kernels.

The Leaves
----------
At the leaves, I plan to support:

* Sphere-traced implicit functions.
* Bisection-traced implicit functions: slower than sphere tracing,
  but supports a larger set of implicit functions.
* Volume data structures, various kinds of discrete signed distance fields.
  Eg, voxel arrays are the most popular on the GPU.
  Eg, efficient and/or accurate volume data structures for representing meshes.
