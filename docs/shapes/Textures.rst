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

``texture`` *tx* *shape*
  Apply the texture *tx* to *shape*.

``texture`` *cfield* *shape*
  Apply the colour field function *cfield* to *shape*.

High Level Constructors
-----------------------

Future Work
-----------
* Create more high level constructors.
* Support natural looking textures, like wood and stone, using fractal noise.
* Look in places like Gimp and Blender for new textures and texture operators.
