Distance Field Operations
=========================
These operations construct a shape from one or more distance fields.
In one or more of the shape arguments, it's the structure of the distance field
that matters, and not just the shape represented by that distance field.

Thus, if you want predictable and repeatable behaviour, you should restrict
distance field arguments to shape expressions that are documented to produce
either an exact or a mitred distance field. In other cases, where the SDF is
only documented as "approximate", the implementation is subject to change.

For all of the distance field operations, we only guarantee to compute a "good"
bounding box estimate if the distance field arguments are exact. Otherwise, the
bounding box may be "bad" (too small to contain the resulting shape),
and the user may need to fix this by calling ``set_bbox``.

* The reason is, for distance field operations, we need a lower bound on the
  ratio by which the distance field underestimates the distance to the boundary
  in order to compute a good bounding box estimate.
  For mitred distance fields in general, there is no lower bound. It's possible
  to determine lower bounds for some shape operations, but not in general.
* Other approaches: Compute this lower bound (if available), and store it in the shape,
  which is added complexity. Or, use an automatic bounding box estimator that uses distance field evaluation.
  
Level Set Operations
--------------------
The level set at ``d`` of a distance field is the set of all points whose distance value is ``d``.
This is also called an isocurve (in 2D) or isosurface (in 3D).

``offset d shape``
  Construct the shape bounded by the level set at ``d`` of the shape argument's distance field.
  
  * d > 0: inflate the shape, blow it up like a balloon.
  * d == 0: no effect.
  * d < 0: deflate the shape.
 
  If the distance field is exact, then you get the "rounded offset" of the shape.
  For positive (negative) ``d``,
  convex (concave) vertices and edges are rounded off
  as if by a circle or sphere of radius ``d``.
  [Also known as Minkowski sum (difference) of a circle or sphere of radius ``d``,
  or dilation (erosion) with a ball of radius ``d`` in Mathematical Morphology.]

  If the distance field is mitred, the result is a "mitred offset".
  Vertices and edges are preserved.

  ``offset`` can be used for debugging, to help visualize the distance field.
  
  Bounding box: If ``shape`` has an exact distance field, then we can compute a
  good bounding box, which is exact if ``shape`` has an exact bounding box
  and if ``d>=0``. If ``shape`` has a mitred or approximate distance field,
  we can only guarantee a good bounding box if ``d<=0``.

``shell d shape``
  Construct a shell of thickness ``d``,
  whose boundaries are plus or minus ``d/2``
  from the original surface or perimeter of ``shape``.

``pancake d shape``
  ``pancake`` converts a 2D shape into a 3D "pancake" of thickness d.
  The edges are rounded. The corners are rounded, if ``shape`` has an exact
  distance field, or sharp, if ``shape`` has a mitred distance field.

Morph Operations
----------------
In which we linearly interpolate between two distance fields.

``morph k (shape1, shape2)``
  Linearly interpolate between the SDFs of shape1 and shape2.
  ``k=0`` yields shape1, ``k=1`` yields shape2.

``loft d (shape1, shape2)``
  Like ``extrude``, except that you specify a lower cross section (``shape1``)
  and a upper cross section (``shape2``)
  and we linearly interpolate between the two SDFs while extruding.
  Similar to Autocad ``loft``.

  `loft` works well if you loft between two roughly symmetrical shapes
  that cover the origin and are centred on the origin.
  When you deviate from that, the behaviour becomes unintuitive.
  
  TODO: bad distance field.

Future Work: high quality morph operations

* "Shape Transformation Using Variational Implicit Functions"
  https://www.cc.gatech.edu/~turk/my_papers/schange.pdf
* "Space Time Blending"
  https://cis.k.hosei.ac.jp/~F-rep/SpaceTimeBlend.pdf  

Nested Distance Fields
----------------------
In which the output of one distance field is fed as input to another distance field.

``perimeter_extrude perimeter cross_section``
  A generalized torus.
  Sweep the origin point of a 2D shape (called ``cross_section``) 
  around the perimeter (distance field zero points) of another 2D shape
  (called ``perimeter``).
  The ``cross_section`` shape is offset from the perimeter based its distance
  from the origin.

  If ``perimeter`` has an exact distance field, then it's like
  a Minkowski Sum of ``cross_section`` with the zero points
  of ``perimeter``, with ``cross_section`` held orthogonal to the XY plane.

  Example: ``torus {major: d1, minor: d2} = perimeter_extrude (circle d1) (circle d2)``

  Example: ``revolve shape = perimeter_extrude (circle 0) shape``

  The behaviour when sweeping around a ``perimeter`` vertex
  depends on whether the ``perimeter`` shape has an exact or mitred distance
  field: the result is a rounded or sharp transition.
  If ``perimeter`` has an approximate distance field, then any
  deformations in that distance field will deform the ``cross_section``.

Blended Union, Intersection and Difference
------------------------------------------
A blended union is a generalized union that smoothly joins nearby objects.
The same code (which I call a "blending kernel") can also define
a blended intersection and a blended difference, which smooth away
new edges created by the intersection or difference.

There are many blending kernels.
In Curv, a blending kernel is a record value with 3 function fields
named ``union``, ``intersection`` and ``difference``.

Blending operations are sensitive to the
structure of the distance fields of their arguments.
A blended union uses the positive distance fields near the surfaces of the
shapes being blended to construct additional material to bridge the gaps
between the two shapes.

---------

The ``smooth r`` blending kernel comprises:

* ``smooth r .union (shape1, shape2)``
* ``smooth r .intersection (shape1, shape2)``
* ``smooth r .difference (shape1, shape2)``

The parameter ``r`` controls the size/radius of the blending band.

*Smooth union* is an implementation of what I call The Elliptic Blend,
since it creates a fillet with an elliptical shape. This blend is fast,
easy to use, and good enough for most purposes.

The Elliptic Blend is a popular blending operation that has been rediscovered or reinvented
many times; every author comes up with a different name and a different algorithm,
but the behaviour is the same:

* "The Potential Method for Blending Surfaces and Corners" by Hoffman and Hopcroft (1987).
  Their blend is controlled by 3 parameters: ``a`` and ``b`` control the blending radius,
  and ``λ`` controls the shape of the fillet. If you set ``a=b=r`` and ``λ=0``
  then you get The Elliptic Blend.
* A special case of the "superelliptic blend" by Rockwood & Owen (1987),
  "Blending Surfaces in Solid Modeling".
  The ellipse is generalized to a superellipse by passing an exponent as argument,
  and there are two ``r`` parameters, one for each shape being blended.
* Independently discovered by Christopher Olah (2011), called "rounded union" in ImplicitCAD.
* Faster implementation by Inigo Quilez as "opBlend", using his "polynomial smooth min" function.
* Even faster implementation by Dave Smith @ Media Molecule (2015), called "soft blend".
* Alternate implementation by MERCURY (same shape but different distance field), called "opUnionRound".

Note that *smooth union* and *smooth intersection* are binary operators:
they aren't associative and don't easily generalize to an arbitrary number of shapes.

Here are circles of diameter 2, combined using *smooth union* with ``r`` values
1.2, 1.8, 2.4, 3.0, 3.6, 4.2, 5.0:

.. image:: ../images/blend.png

This looks very similar to the older "blobby objects" / "soft objects" / "`Metaballs`_" technique.
The Elliptic Blend is more general, since it works with all geometric shapes, not just circles and spheres.
But it's also less general, since it doesn't blend 3 or more shapes together
in an order-independent way.

.. _`metaballs`: https://en.wikipedia.org/wiki/Metaballs

Smooth blends can produce the artistic effect of "fillets" and "rounds" from mechanical engineering.
Here are *smooth union*, *smooth intersection* and *smooth difference*
applied to a unit cube and a cylinder with ``r=.3``:

.. image:: ../images/smooth_blends.png

Here's the appearance of a fillet (with the same ``r``) for different
angles: 90°, 45°, 135°.

.. image:: ../images/fillet_angles.png

At 90°, the fillet is a quarter-circle with radius ``r``.
At other angles, the fillet deforms to an ellipse.
This might be bad for engineering, if you need a constant radius fillet,
but it's good if you are animating an organic form (like a leg attached to a torso),
and you want a constant-area fillet that looks realistic as the joint is animated.

Here's a fillet of a butt joint, same parameters as above.
To get a rounded fillet in this example, the rectangles must have exact distance fields,
so I used ``rect.exact``. This shows that the bounding box of *smooth union* can be
bigger than the bounding box of ``union``. It also shows an example of a "bulge".

.. image:: ../images/butt_fillet.png

The "bulge" behaviour of the Elliptic Blend is considered undesirable by many people,
and there are more sophisticated blends available that avoid it.
The bulge can also be used artistically: Quilez has used it to create knee and knuckle joints
in cartoonish creatures.

As a special case, ``smooth r .union (s, s)`` is the same as ``offset (r/4) s``.
This is specific to my current code. This seems to be the worst case
for bounding box inflation, so we can use this to compute bounding boxes.

Distance field: approximate. Haven't seen a bad distance field during testing.

Bounding box: approximate.

TODO: enhance ``smooth`` blending kernel to support N-ary blends.

------

``chamfer r`` is a blending kernel that makes a 45-degree chamfered edge (the diagonal of a square of size ``r``).

Distance field: approximate.

Bounding box: approximate.

TODO: support N-ary blends.

------

TODO: fancy blending kernels from MERCURY, like ``columns`` and ``stairs``.
See: http://mercury.sexy/hg_sdf/

TODO: investigate advanced blending primitives from

* "A Gradient-Based Implicit Blend" (2013),
  http://citeseerx.ist.psu.edu/viewdoc/download?doi=10.1.1.592.5451&rep=rep1&type=pdf
  "Suppressing bulges when
  two shapes merge, avoiding unwanted blending at a distance, ensuring that
  the resulting shape keeps the topology of the union, and enabling sharp
  details to be added without being blown up. The key idea is that field functions
  should not only be combined based on their values, but also on their
  gradients."
* "Extrusion of 1D implicit profiles: Theory and first application" (2001)
  https://www.irit.fr/recherches/VORTEX/publications/rendu-geometrie/IJSM2001_Barthe_et_al.pdf
