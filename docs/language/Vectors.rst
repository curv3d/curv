Points and Vectors
------------------
Geometric points and vectors are represented by a list of numbers.

Vec2 and Vec3
  ``[x,y]`` and ``[x,y,z]`` represent 2 and 3 dimensional points,
  and also 2 and 3 dimensional vectors.
  
  ``is_vec2 x``
    True if ``x`` is a 2 dimensional vector or point.
  
  ``is_vec3 x``
    True if ``x`` is a 3 dimensional vector or point.
  
  ``p.[X]``, ``p.[Y]``, ``p.[Z]``
    The X and Y components of a Vec2 or Vec3.
    The Z component of a Vec3.
    Note: ``X=0``, ``Y=1`` and ``Z=2``.

``mag v``
  The magnitude of a vector ``v`` (sometimes called the length or the Euclidean norm).
  Equivalent to ``sqrt(sum(v^2))``.

``normalize v``
  Normalize a vector: convert it to a unit vector with the same direction.
  Equivalent to ``v / mag v``.

``dot[v1,v2]``
  Vector dot product (a special case of the array dot product).
  The result is a number. Same as ``sum(v1*v2)``.
  
  * Equivalent to ``mag v1`` × ``mag v2`` × *cos* θ,
    where θ is the angle between v1 and v2 and 0° ≤ θ ≤ 180°.
  * If v1 and v2 are at right angles, dot[v1,v2] == 0.
    If the angle is < 90°, the result is positive.
    If the angle is > 90°, the result is negative.
  * If v1 and v2 are unit vectors, then acos(dot[v1,v2]) is the angle
    between the vectors, in the range [0,tau/2].
    Note that this is expensive, and inaccurate for small angles.
    See ``perp`` for an alternative.
  * The scalar projection (or scalar component) of vector a in the direction of unit vector b
    is ``dot[a,b]``.
  
``phase v``
  The phase angle of a 2D vector, in the range ``tau/2`` to ``-tau/2``.
  This is the angle that the vector makes with the positive X axis,
  measured counter-clockwise.

``cis theta``
  Convert a phase angle to a unit 2D vector.

``perp v``
  Rotate a 2D vector by 90 degrees (tau/4) counterclockwise.
  Multiply a complex number by ``i``.
  ``perp[x,y]`` is equivalent to ``[-y,x]``.

  ``dot[perp v1, v2]`` is the "perp-dot product" of 2D vectors:

  * Equivalent to ``mag v1`` × ``mag v2`` × *sin* θ,
    where θ is the angle between v1 and v2, and -90° ≤ θ ≤ 90°.
    A positive angle indicates a counterclockwise rotation from v1 to v2.
  * Result is 0 if the vectors are colinear, >0 if the smaller angle from v1 to v2
    is a counterclockwise rotation, or <0 for a clockwise rotation.
  * The absolute value is the area of the parallelogram with the vectors as two sides,
    and so twice the area of the triangle formed by the two vectors.
  * ``z=[dot[a,b],dot[perp a,b]]`` is a complex number that represents the signed angle
    between the vectors. ``phase z`` is the signed angle.
    ``cmul[pt, normalize z]`` rotates a point around the origin through that angle.
  * See 'The Pleasures of "Perp-Dot" Products', Graphics Gems IV.
  

``cross[v1,v2]``
  `Cross product`_ of 3D vectors, which is a 3D vector.
  
  * If v1 and v2 are colinear, or if either is [0,0,0], then the result is [0,0,0].
  * Otherwise, the result is a vector that is perpendicular to both v1 and v2
    and thus normal to the plane containing them.
  * The magnitude of the cross product equals the area of a parallelogram
    with the vectors for sides. For two perpendicular vectors, this is the product of their lengths.
  
.. _`Cross product`: https://en.wikipedia.org/wiki/Cross_product
