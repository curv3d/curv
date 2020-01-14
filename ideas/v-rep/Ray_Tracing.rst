Ray Tracing an Implicit Function
================================
An implicit function is a continuous function that defines a 3D shape
by mapping an arbitrary point (x,y,z) to a number that is < 0 if the point
is inside the shape, 0 if the point is on the boundary, or > 0 if the point
is outside.

For ray tracing, an implicit function is not enough to reliably find the
boundary of a shape. You need some additional structure.

Here is why. Starting from a camera position, you
need to trace a ray in a particular direction until it intersects the closest
boundary of the shape. With just an implicit function, and no other structure,
there's no reliable way to find that closest intersection. You could sample
along the ray at regular intervals and look for a sign change, which would
indicate a shape boundary within the interval. But you might have skipped over
a small feature enclosed by an earlier interval.

Grid Tracing
------------
A method that works for tracing algebraic functions. Used by many 3D
equation plotters, which can plot functions that Curv cannot.
This will at least give me parity with equation plotters (and libfive)
in terms of what SDFs I can render, and might be a fallback when faster
algorithms fail.

Sample along the ray at regular intervals and look for a sign change, which
indicates a shape boundary within the interval. But you might have skipped over
a small feature enclosed by an earlier interval. And it's slow.

Requires a finite bounding box, and requires user to specify step size
when stepping inside that bounding box.

glsl-function-grapher by Michael Firmin on github

Sphere Tracing
--------------
Sphere tracing is the most popular method for ray-tracing implicit functions.
It supports a wide range of shapes and operations; it is fast
(especially on the GPU), easy to implement, and it is robust.
The demos on `<shadertoy.com>`_ are a well known application.

Sphere tracing requires the implicit function to be a Signed Distance Function
with Lipschitz(1) continuity, abbreviated as L1-SDF.

It can be difficult or expensive to find L1-SDF versions
of some notable implicit functions:

* Many algebraic functions.
* The result of applying a deformation (non-similarity transformation).

See also: `<Deriving_Distance_Functions.rst>`_.

Enhanced Sphere Tracing
-----------------------
http://erleuchtet.org/~cupe/permanent/enhanced_sphere_tracing.pdf
https://www.shadertoy.com/view/4tVXRV

Method of marching further than 1.*radius, and then checking if last 2 circles
overlap, and jumping back if they do not overlap.

This shader uses "enhanced spheretracing" that is more optimistic, in favor of
convex surfaces, but it becomes less efficient of the surface has too many
convex valleys. See figure 3 of paper: it tends to measure cones, and not just
spheres. The overhead is one more subtraction, 1 more distance variable and 1
state variable within the marching loop. It does not merge too well with a "glow
accumulating marcher" and likely merges well with a "cheap AO accumulating
marcher" (that just buffers the last 4 radii for very lazy AO, that is to the
camera and not a surface normal). Both of these are common, one accumulates
illumination/fog, one rotates 4 buffers for AO.

Newton Tracing
--------------
Newton's method is a root finding method that can be incorporated into
ray tracing algorithms. It requires the implicit function to be C1 continuous.

C1 continuity is an inconvenient requirement, since geometric primitives that
create sharp edges and corners, like cube and union, are not C1 continuous. We
would need to substitute continuous approximations. Fractals, represented by
iterated function systems, are not generally C1 continuous. In fact, Sphere
Tracing was originally invented to solve the problem of ray tracing fractals.

Bisection Tracing
-----------------
In addition to Lipschitz(1) continuity and C1 continuity, a third form of
additional structure is interval arithmetic. The implicit function is
supplemented by a related function that maps domain intervals over X, Y and Z
of the implicit function onto a range interval. If the source code of the
function is available (which is always the case in Curv), then the interval
function can be automatically generated. And this trick works for
any computable function.

Interval arithmetic would be a powerful extension to Curv.
While implicit functions are test functions for classifying
points in space as inside, on or outside an object, interval
arithmetic allows us to extend those tests to whole chunks of
space at once. For example, you can automatically compute bounding boxes.
And interval arithmetic can be used as an alternative method in ray tracing
for discarding rays that don't hit the shape (that's a topic for future
investigation).

So, I'd like to supplement sphere tracing with another ray-tracing method
based on interval arithmetic / inclusion algebra. This is expected to be slower,
but it will support arbitrary implicit functions.

The method I've chosen combines interval arithmetic with bisection.
This has been implemented multiple times, and several papers
explore ways to optimize it. It was reported to give real-time ray-tracing
performance on a GPU in 2008, and is probably the easiest of the interval
arithmetic methods. It works for any computable function. To get best results,
each shape is tuned by specifying an epsilon parameter, which is not ideal,
but is at least a lot easier than creating an implicit function with a
Lipschitz bound.

References:

* "CSG Operations of Arbitrary Primitives with Interval Arithmetic and Real-Time Ray Tracing" (2008)
  https://www.cs.utah.edu/~aek/research/csgimplicits.pdf
* "RealSurf – A GPU-based Realtime Ray Caster for Algebraic Surfaces" (2009)
  They use bisection without interval arithmetic to raytrace algebraic surfaces,
  and they get very high frame rates.
  Unlike sphere tracing, it's not guaranteed to work:
  "Rendering errors typically occur for surfaces of high degree or near
  certain types of singularities".

Bisection is a simple but slow root finding algorithm, similar to binary search:
it has linear convergence. Newton's method is faster (quadratic convergence),
but as mentioned above, now we need the implicit function to be C1 continuous.
The "Realsurf" paper claims that bisection is more numerically stable than
faster rootfinding methods when using 32 bit floats.

More Methods
------------
Real-Time Ray-Tracing of Implicit Surfaces on the GPU
http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.417.5616&rep=rep1&type=pdf

Our adaptive marching points algorithm can ray-trace arbitrary implicit surfaces
without multiple roots, by sampling the ray at selected points till a root is
found. Adapting the sampling step size based on a proximity measure and a
horizon measure delivers high speed. The sign-test can handle any surface
without multiple roots. The Taylor-test that uses ideas from interval analysis
can ray-trace many surfaces with complex roots. Overall, a simple algorithm that
fits the SIMD architecture of the GPU results in high performance. We
demonstrate the ray-tracing of algebraic surfaces up to order 50 and
non-algebraic surfaces including a Blinn’s blobby with 75 spheres at better than
interactive framerates.

Hybrid V-Rep
------------
Since interval arithmetic is usually slower than sphere tracing, I intend to
gain performance by using a hybrid representation. There is a bounding volume
hierarchy, with sphere-traced or bisection-traced objects at the leaves. Each
tree node is labeled as to whether it can be sphere-traced or must be
bisection-traced. If you restrict your model to only Lipschitz(1) primitives
(the only choice available in Curv 0.0) then only the faster sphere-tracing
algorithm will be used, and there is gradual performance degradation as
bisection-traced objects are introduced. See `<Hybrid.rst>`_.
