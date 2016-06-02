= SGDL and the Arithmetic of Forms
This is an innovative F-Rep geometry engine developed by some guys in Montreal.
It's an original remix of functional representation, using homogenous
coordinates and projective geometry.
The representations used for points, lines, planes and solids may have
advantages over the traditional approach.
* There is a natural representation for points, lines, etc at infinity,
  which simplifies some algorithms (and speeds them up by removing the need
  for conditional tests).
* The 3D point [x,y,z]
  is represented in homogeneous coordinates as [x*k,y*k,z*k,k].
  The 4 coordinates can be integers, and you get the benefits of
  exact arithmetic with rational numbers, but at lower cost.
  It's claimed that the SGDL kernel only uses addition, subtraction and
  multiplication on integers, no division.
* Quadrics are the primitive solids in SGDL. Even F-Rep uses bounding quadrics
  instead of bounding boxes.
  * Quadric solids (including spheres, ellipsoids, cylinders, cones, planes,
    etc) are represented in projective geometry as a 6x4 matrix (6 points),
    which are transformed by matrix multiplication. (Wikipedia says 4x4, but
    SGDL actually uses 6x4.) SGDL's quadrics (le quartique) are closed over
    projective transformations (affine transformations + perspective, I think),
    which increases the number of possibilities, and may account for the 4 vs 6
    discrepancy.
  * An ellipsoid with 2 of the 3 axes at infinity is an infinite solid with
    two parallel planar surfaces: intersect 3 of these at right angles to get a
    cuboid. Arbitrary curved surfaces are approximated using quadric patches
    (CSG of multiple quadric solids).
  * You can perform exact geometric calculations on quadrics. The SGDL
    propaganda mentions "axioms of incidence", "passage through control points"
    and "tangency of points of contact".
  * The computations for intersecting lines and surfaces with quadrics are
    cheaper than for parametric splines.
    One of my main F-Rep sources claims that ellipsoids are difficult to define
    and expensive to compute; maybe quadric techniques are an alternative?
  * According to SGDL propaganda, "Quartics open an amazing universe of forms,
    some of them unknown by now."
* F-Rep is used for CSG. F-Rep functions (called density functions) map a
  3-space point onto 0, 1 or 2 if the point is outside, on the boundary, or
  inside. Union and intersection are max and min, and other arithmetic
  operations, like multiplication (?), are used for other effects.

Related work: Stolfi primitives for oriented projective geometry.
"Many geometric algorithms become simpler, more general, and more efficient
when recast in the language of projective geometry."
https://graphics.stanford.edu/courses/cs348a-09-fall/Papers/Stolfi_Primitives_DECSRC_Report.pdf

There is limited information available.

Original thesis in French:
"L’arithmétique des formes: une introduction à la logique de l’espace"
http://www.collectionscanada.gc.ca/obj/s4/f2/dsk2/ftp03/NQ33078.pdf

More details here, also French:
http://www.grcao.umontreal.ca/data/pdf/001sf001.pdf

SGDL is a pure functional language for 3D geometry, embedded in Scheme.

"SGDL-Scheme: a high level algorithmic language
for projective solid modeling programming"
by Jean-Francois Rotge
http://repository.readscheme.org/ftp/papers/sw2000/rotge.pdf

It uses functional representation using distance fields.
But, I don't understand the encoding. There's an example in SGDL-Scheme
which isn't explained.

In the thesis, a "distance function" is 0 outside, 1 at the boundary, 2 inside.

"Quadrics" (quadrique) seem to be important. Wikipedia discusses quadrics
using projective geometry: https://en.wikipedia.org/wiki/Quadric
The sides of a polyhedron are represented by degenerate quadric surfaces(?).

It uses a clever encoding of coordinates, which is equivalent to
[x,y,z] using rational numbers, but faster. The encoding is homogenous
coordinates using projective and affine geometry, [x,y,z,d], where all 4
coordinates are integers, and d is the denominator of x,y, and z.
The kernel uses addition and multiplication of integers, no division.
If d==0 then the point lies at infinity, and the algebra relies on this
to make algorithms work consistently and uniformly without using if statements
to test for exceptional values.

The reasons given for using an exact representation are the same as those
given by CGAL.

The corporate propaganda at sgdl.com (defunct since 2007)
claims the kernel is very fast and uses very little memory.
