Textures
========
A texture is an infinite colour pattern that can be applied to a shape,
giving it colour. Textures can be 2D or 3D, and they can be animated.
When you apply a 3D texture to a 3D shape, you are specifying the colour of each
interior point of the shape, as well as the surface points.

Textures are graphical values, which can be displayed in the graphics window.

.. _`colour value`: Colour.rst
.. _`shape`: Shapes.rst

Basic Operations
----------------
``make_texture`` *colour*
  Construct a single-colour texture.
  Eg, ``make_texture red``.

``make_texture`` *cfield*
  Construct a variegated texture from a colour field function,
  which maps the parameters ``(x,y,z,t)`` onto a colour value.
  2D colour fields ignore the ``z`` parameter.
  A colour field that uses the time parameter ``t`` is animated.
  
  This is a very powerful interface.
  You can do a lot in a small amount of code.
  Eg::
  
    make_texture((x,y,z,t)->sRGB.hue(cos x * cos y * cos z + t/8 `mod` 1))

``make_texture (ifield, cmap)``
  Construct a texture by composing an intensity field with a colour map.

``texture`` *tx* *shape*
  Apply the texture *tx* to *shape*.

``texture`` *cfield* *shape*
  Apply the colour field function *cfield* to *shape*.

``texture (ifield, cmap) shape``
  Construct a texture by composing an intensity field with a colour map,
  then apply that texture to the shape.

Intensity Field Operations
--------------------------
``show_ifield ifield``
  Display an intensity field in the graphics window.

``i_linear d``
  This 2D ifield is a series of vertical stripes of width ``d``;
  each stripe is a gradient going from 0 to 1 along the X axis.

``i_radial n``
  This 2D ifield is a radial starburst pattern containing ``n`` pie-slice
  shaped gradients that go from 0 to 1 along arcs that are a constant distant
  from the origin.

``i_concentric d``
  This 2D ifield contains concentric rings of thickness ``d``.
  Each ring is a gradient from 0 to 1 as the distance increases from the origin.

``i_gyroid``
  This 3D ifield is constructed from the distance field of the gyroid.
  The value is 0.5 on the gyroid surface, and reaches 0 or 1 at the extrema,
  which are points at maximum distance from the gyroid surface.

``i_animate t ifield``
  Animate an ifield by cycling the values with a period of ``t`` seconds.

Texture Transformations
-----------------------
Textures may be transformed using `Transformations`_ and `Repetition Operators`_.

.. _`Transformations`: Transformations.rst
.. _`Repetition Operators`: Repetition.rst

Future Work
-----------
* Create more high level constructors.
* Support natural looking textures, like wood and stone, using fractal noise.
* Colour transformations, which transform the colours in a texture.
* Look in places like Gimp and Blender for new textures and texture operators.
