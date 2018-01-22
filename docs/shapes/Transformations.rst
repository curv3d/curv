Transformations
===============
A *geometric transformation* transforms one shape into another by mapping each point in the shape onto another point in space.

A `similarity transformation`_ preserves angles and distance ratios. This class of transformation comprises translations, rotations, reflections, and uniform scaling. The structure of the distance field is preserved, so there are no limitations on where similarity transformations can be used.

.. _`similarity transformation`: https://en.wikipedia.org/wiki/Similarity%20%28geometry%29

A *deformation* is a more general transformation. You can stretch, twist or bend the shape, eg stretching a sphere into an ellipsoid, or bending a cylinder into a torus. In Curv 0.0, deformations deform the distance field, which complicates and limits their use.

Deformations also work on textures (infinite 2D and 3D shapes whose role is to carry a colour pattern), without limitations. Deformations are a powerful tool for procedural texture generation.

Underlying each geometric transformation is a transformation function that maps each point in space onto another point.
This transformation function is required to have an inverse which is also a transformation.
(A transformation cannot map two different points onto the same point.)

Similarity Transformations
--------------------------
Transformations of 2D and 3D shapes which preserve angles and distance ratios.

These transformations preserve the structure of the distance field.
If the input has an exact or mitred distance field, the output is also exact or mitred.
As a result, there are no limitations on where similarity transformations can be used.

``move (dx,dy) shape``
  Translate a 2D or 3D shape across the XY plane.

``move (dx,dy,dz) shape``
  Translate a 3D shape.

``rotate angle shape``
  Rotate a 2D or 3D shape around the Z axis, counterclockwise,
  by an angle measured in radians.

``rotate {angle, axis} shape``
  Rotate a 3D shape around the specified axis, counterclockwise,
  by an angle measured in radians.

``rotate quaternion shape``
  TODO

``reflect v shape``
  Reflect the shape through a line or plane passing through the origin,
  whose normal vector is ``v``. If ``v`` is embedded in the X-Y plane (v[Z]==0)
  then this operation applies to either 2D or 3D shapes, otherwise the shape
  must be 3D.

  For example, ``reflect X_axis shape`` reflects ``shape`` across the Y axis
  (in 2D) or the Y-Z plane (in 3D).
  Each point ``[x,y,z]`` in ``shape`` is mapped to ``[-x,y,z]``.

``scale k shape``
  Isotropic scaling by a scale factor of ``k`` of a 2D or 3D shape.

``at p t shape``
  Apply a transformation ``t`` to a shape,
  treating the point ``p`` as the origin point of the transformation.
  
  Example: ``square 2 >> at (1,1) (rotate(45*deg))``
  rotates the square around the point (1,1).

``align alignspec shape``
  TODO: Using the shape's bounding box,
  translate the shape to align it relative to the origin,
  as specified by ``alignspec``.
  
  ``alignspec ::= {x: aspec, y: aspec, z: aspec}``
  
  Each field of alignspec is optional, and aspec is one of:
    
  * ``above d`` -- a point that is ``d`` above the top of the shape's bounding box.
  * ``below d`` -- a point that is ``d`` below the bottom of the shape's bounding box.
  * ``within k`` -- ``k`` is between -1 (the bottom of the bounding box)
    and +1 (the top of the bounding box). 0 is the centre.
  * ``centre`` -- centre of the shape's bounding box, same as ``within 0``.
    
  Eg, ``align {z: above 0}`` aligns the bottom of the shape with ``z==0``.
  
  See also: General Library of Relativity
  https://github.com/davidson16807/relativity.scad/wiki

Shape Deformations
------------------
General transformations of 2D and 3D shapes which don't preserve angles and distance ratios.
These are powerful operations which allow you to deform a shape, as if it were made of clay.

In Curv 0.0, deformations deform the distance field, which limits and complicates their use.
(This is an area for future research and improvement.)

* The output may not be appropriate as input to distance field operations.
* The anistropic scale operator produces an approximate distance field.
  The remaining operations produce "bad" distance fields, which must be
  corrected using the ``lipschitz`` operator in order for the sphere-tracing
  algorithm in the previewer to work.

``scale (kx, ky) shape``
  Anisotropic scaling of a 2D or 3D shape across the XY plane.

``scale (kx, ky, kz) shape``
  Anisotropic scaling of a 3D shape.

``shear_x kx shape``
  2D horizontal shear, defined on 2D and 3D shapes, mapping ``(x,y,z)`` to ``(x + kx*y, y, z)``.
  If ``kx>0``, this maps a unit square to a right-tilting parallelogram of height 1 and width ``1+kx``.
  So ``shear_x 1`` will double the width of a square, and ``shear_x 2`` will triple the width.
  
  Want to specify the shear factor as a tilt angle, like SVG skewX(a) or CSS skew(a)?
  Use ``shear_x(tan a)``.
  The tilt angle is measured clockwise from the +Y axis, ``a==0`` means no tilt.
  
  TODO: distance field is bad.
  
``shear_xy (kx,ky) shape``
  3D horizontal shear, defined on 3D shapes, mapping ``(x,y,z)`` to ``(x + kx*z, y + ky*z, z)``.
  
  TODO

``local_taper_x {range:(y0,y1), scale:(kx0, kx1)} shape``
  Local 2 dimensional taper along the Y axis, between y==y0 and y==y1.
  When y<=y0, x values are scaled by the factor kx0.
  When y1<=y, x values are scaled by the factor kx1.
  When y0<y<y1, x values are scaled by a factor that is a linear ramp
  between kx0 and kx1.

``local_taper_xy {range:(z0,z1), scale:([kx0, ky0], [kx1, ky1])} shape``
  Local 3 dimensional taper along the Z axis, between z==z0 and z==z1.
  When z<=z0, x and y values are scaled by the factors kx0 and ky0.
  When z1<=z, x and y values are scaled by the factors kx1 and ky1.
  When z0<z<z1, x/y values are scaled by factors that are a linear ramp
  between kx0-kx1/ky0-ky1.

``twist d shape``
  Twist a 3D shape around the Z axis. One full revolution for each ``d`` units along the Z axis.
  Lines parallel to the Z axis will be twisted into a helix.
  
  TODO: distance field is bad.

``bend d shape``
  Take the upper half of the XY plane between X==-d/2 and X==d/2,
  and wrap that radially around the origin to cover the XY plane,
  by mapping rectangular coordinates in the source region to polar coordinates
  in the target region.
  
  TODO: bad distance field.

Texture Deformations
--------------------
A texture is a 2D or 3D colour pattern, represented by an infinite, space filling shape
whose colour field represents the texture.

Any transformation can be applied to a texture: that's a tool for creating procedural textures.
Some deformations distort the distance field too badly to be applied to 3D shapes, but these
deformations can still be applied to textures:

``swirl {radius, strength} shape``
  This is a classic 2D image transformation, based on an API in the Python scikit-image library,
  where ``strength`` is the amount of swirl, and ``radius`` indicates the swirl extent.
  
  * ``strength``: 0 means no effect, positive means a clockwise swirl, negative means
    a counterclockwise swirl.
  