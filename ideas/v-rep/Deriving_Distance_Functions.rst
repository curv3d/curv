Deriving Distance Functions
===========================
Sphere tracing requires a distance function to be Lipschitz(1) continuous.
It can be difficult and frustrating to derive Lipschitz(1) versions of
implicit functions, for those not versed in the higher mathematical arts.

The ``offset`` operator can construct the rounded offset of a shape, which is
an important operation, but the input must be an exact Euclidean distance
function. That's even more difficult to derive than Lipschitz(1).

Here are some meditations on the topic of deriving distance functions.

Tom Duff
--------
In 1992, Tom Duff wrote:

"Lipschitz constants can be computed by interval evaluation
of a function’s derivatives, and those bounds converted into
bounds on the original function using the mean value theorem. In
this light it is perplexing that Kalra and Barr [13] put the question
of ‘identifying ... useful implicit functions and computing
Lipshitz constants’ for them first on their list of important
problems to attack."

Does this advice work? I don't know. Duff mentions "derivatives": does this
mean that the implicit function must be C1 continuous for his proposed method
to work?

Source:
"Interval Arithmetic and Recursive Subdivision for
Implicit Functions and Constructive Solid Geometry"
http://fab.cba.mit.edu/classes/S62.12/docs/Duff_interval_CSG.pdf

Per-Olof Persson
----------------
According to Per-Olof Persson,
you can convert an arbitrary implicit function to an exact Euclidean
distance function (which is also Lipschitz(1)), but his method uses
iterative root finding, which is expensive to evaluate.

Source:
http://persson.berkeley.edu/thesis/persson-thesis-color.pdf
"Mesh Generation for Implicit Geometries" by Per-Olof Persson
Section 2.4, pages 31-32.

I don't understand the math, but Newton's method is mentioned.
Newton's method is a root finding method; it requires the target function
to be C1 continuous.
This is an inconvenient requirement, since geometric primitives that create
sharp edges and corners, like cube and union, are not C1 continuous. We would
need to substitute continuous approximations.
Or use a root finding method that doesn't require C1 continuity.

I hypothesize that it's possible to use this method to write a Curv function
that converts an arbitrary implicit function to an exact Euclidean distance
function.

Lipschitz Constants for Deformations
------------------------------------
Deformations (non-similarity transformations) like twist, bend and swirl are
one case where I have difficulty figuring out Lipschitz constants.

Stijn Stiefelhagen does this by first computing the L2 norm of the
Jacobian matrix of the deformation. See pages 33 and 34 of his thesis:

"Lipschitz versus interval arithmetic in ray tracing implicits"
https://pure.tue.nl/ws/files/46965615/658568-1.pdf

John C. Hart notes that: The Lipschitz constant of an arbitrary linear
transformation is found by the power method,
which iteratively finds the largest eigenvalue of
a matrix (Gerald and Wheatley 1989 "Applied numerical analysis.").

Ray Tracing Arbitrary Implicit Functions
----------------------------------------
Libfive (a competitor to Curv) can render arbitrary implicit functions.
It is not constrained to only rendering Lipshitz(1) distance functions, the
way that Curv 0.0 is. Libfive uses interval arithmetic for rendering.

Of course, Libfive has other constraints. It can't render large objects, or
objects with fine detail. It doesn't support ray tracing.

Some people on the web have used interval arithmetic to ray trace arbitrary
implicit functions using a GPU, and they were getting real time frame rates
using 2008 GPU hardware. This avoids the need for advanced users to deal with
the math referenced above, so I'm interested in pursuing this.
See: `<Ray_Tracing.rst>`_.
