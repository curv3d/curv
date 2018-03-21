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

* "CSG Operations of Arbitrary Primitives with Interval Arithmetic and Real-Time Ray Tracing"
  https://www.cs.utah.edu/~aek/research/csgimplicits.pdf

Bisection is a simple but slow root finding algorithm, similar to binary search:
it has linear convergence. Newton's method is faster (quadratic convergence),
but as mentioned above, now we need the implicit function to be C1 continuous.

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
