Colour
======

Curv provides operations for constructing colour values,
displaying colours on the screen, and applying colours to geometric shapes.

For example:

* ``red`` is a colour value.
* ``show_colour red`` displays the colour red in the graphics window.
* ``colour red cube`` is a red cube.

Internal Representation (Linear RGB)
------------------------------------
A colour value is an (R,G,B) triple, where:

* The R, G and B components represent the linear intensity
  of red, green and blue light in the colour.
* Each component is a number between 0 and 1 inclusive.

This representation (called linear RGB) is a low-level representation
that is useful internally, for mixing colours and performing computations
within the 3D lighting model, but it is not user friendly.
You don't want to construct linear R,G,B values by hand, you want to use
high level interfaces to construct colour values for you.

Colour Names
------------
Curv 0.0 has a limited set of predefined colour names.

* ``red``
* ``orange``
* ``yellow``
* ``green``
* ``cyan``
* ``blue``
* ``magenta``
* ``white``
* ``black``

Colour Spaces
-------------
High level operations for constructing arbitrary colour values.

``sRGB (r,g,b)``
  Construct a colour value using red, green and blue values in the
  standard `sRGB colour space`_, which is the default colour space
  of the world wide web. Each colour component is in the range 0 to 1.

.. _`sRGB colour space`: https://en.wikipedia.org/wiki/SRGB

``webRGB (r,g,b)``
  Another way to specify sRGB colours, where the colour components are
  in the range 0 to 255. A web colour like "#FFDAB9" (PeachPuff)
  can be transcribed in Curv like this: ``webRGB(0xFF,0xDA,0xB9)``.

``sHSV (hue,saturation,value)``
  A more user-friendly way to specify sRGB colours.

  ``hue``
    This represents a pure, fully saturated colour,
    including all of the (representable) colours in the rainbow.

    === =======
    0   red
    1/6 yellow
    1/3 green
    1/2 cyan
    2/3 blue
    5/6 magenta
    1   red
    === =======

  ``saturation``
    Distance from white/grey/black, from 0 to 1.

    * A ``saturation`` of 0 constructs a monochrome greyscale colour
      based on the ``value``, ignoring the ``hue``. So ``sHSV(h,0,v)``
      is white if v==1, medium grey if v==0.5, and black if v==0.
    * A ``saturation`` of 1 constructs a "shade" (a mixture of a pure
      colour and black), where ``hue`` is the pure colour,
      and ``value`` is the distance from black.

  ``value``
    The brightness, from 0 to 1.

    * If the ``value`` is 0, then the resulting colour is black,
      ignoring the hue and saturation.
    * A ``value`` of 1 constructs a "tint" (a mixture of a pure colour
      and white), where ``hue`` is the pure colour,
      and ``saturation`` is the distance from white.
