Colour
======

Curv provides operations for constructing colour values,
displaying colours on the screen, and applying colours to geometric shapes.

For example:

* ``red`` is a colour value.
* ``show_colour red`` displays the colour red in the graphics window.
* ``cube >> colour red`` is a red cube.

Colour Spaces
-------------
A *colour space* maps a set of coordinates (usually called R, G and B)
onto a set of absolute, colorimetrically quantified colours.
Each Curv colour value is constructed using a specific colour space,
and denotes an absolute colour.

The *gamut* of a colour space is the set of absolute colours it contains.
The gamut of most popular colour spaces is smaller than the set of colours
that can be perceived by the human visual system.

The `sRGB colour space`_ has been the industry standard for the
computer graphics industry for many years. It's the colour space of
most older monitors. It's the default colour space for images and
for the world wide web.

.. _`sRGB colour space`: https://en.wikipedia.org/wiki/SRGB

Newer displays support larger colour spaces with wider gamuts than sRGB.
Newer Apple products support the *DCI-P3* colour space,
and the *Rec.2020* colour space in the UHDTV standard is even larger.

For now, Curv only supports sRGB. However, support for other colour spaces
is planned, both to fully support modern display hardware, and to accurately
describe and display coloured shapes intended for 3D printing, since colour
3D printers have very different gamuts and colour spaces than display hardware.

Colour spaces are represented by Curv values.
For now, we just have one such value, called ``sRGB``.

Constructors
------------
High level operations for constructing colour values.

``sRGB [r,g,b]``
  Construct a colour value using red, green and blue coordinates in the
  sRGB colour space. Each colour component is in the range 0 to 1.

``webRGB [r,g,b]``
  Another way to specify sRGB colours, where the colour components are
  in the range 0 to 255.
  A `web colour`_ like PeachPuff (#FFDAB9 or rgb(255,218,185))
  can be transcribed in Curv like this: ``webRGB[0xFF,0xDA,0xB9]``
  or ``webRGB[255,218,185]``.

.. _`web colour`: http://encycolorpedia.com/

``sRGB.hue h``
  A hue is a pure colour with no white or black mixed in.
  ``sRGB.hue`` is a colour map that can construct any of the hues in the sRGB colour space.
  ``h`` is a number between 0 and 1:

  ===== ============
  0     red -- the red primary of the sRGB colour space
  1/12  orange
  1/6   yellow
  1/4   chartreuse
  1/3   green -- the green primary of the sRGB colour space
  5/12  spring green
  1/2   cyan
  7/12  azure
  2/3   blue -- the blue primary of the sRGB colour space
  3/4   violet
  5/6   magenta
  11/12 rose
  1     red
  ===== ============

``sRGB.grey i``
  A greyscale colour map containing all of the neutral colours in the sRGB colour space.
  ``i`` is between 0 and 1, where 0 is black and 1 is white.

``sRGB.HSV [hue,saturation,brightness]``
  HSV (also known as HSB) is a popular colour model supported by many
  graphics languages.
  ``sRGB.HSV`` is a more user-friendly way to specify sRGB colours.
  The three arguments are:

  ``hue``
    A number between 0 and 1, specifying a hue (a pure colour). See ``sRGB.hue``.

  ``saturation``
    Saturation is the distance from a neutral colour (white/grey/black), in the range 0 to 1.
    Turning down the saturation leaches out the hue and brings the colour
    closer to neutral.

    * A ``saturation`` of 0 constructs a neutral greyscale colour
      based on the ``brightness``, ignoring the ``hue``. So ``sRGB.HSV[h,0,b]``
      is white if b==1, medium grey if b==0.5, and black if b==0.
    * A ``saturation`` of 1 constructs a "shade" (a mixture of a pure
      colour and black), where ``hue`` is the pure colour,
      and ``brightness`` is the distance from black.

  ``brightness``
    Brightness (aka "value") is the distance from black, from 0 to 1.
    Turning down the brightness makes the colour dimmer and closer to black.

    * If the ``brightness`` is 0, then the resulting colour is black,
      ignoring the hue and saturation.
    * A ``brightness`` of 1 constructs a "tint" (a mixture of a pure colour
      and white), where ``hue`` is the pure colour,
      and ``saturation`` is the distance from white.

For convenience, there is a limited set of predefined sRGB colour names:

* ``red``
* ``yellow``
* ``green``
* ``cyan``
* ``blue``
* ``magenta``
* ``white``
* ``black``

Applying Colours to Shapes
--------------------------
``show_colour`` *colour*
  Display *colour* in the graphics window.

``colour`` *colour* *shape*
  Apply a colour to a shape.

Internal Representation (Linear RGB)
------------------------------------
A colour value is an [R,G,B] triple, where:

* The R, G and B components represent the linear intensity
  of red, green and blue light in the colour.
  This is different from sRGB, which uses a non-linear encoding.
* Each component is a number between 0 and 1 inclusive.

This representation (called linear RGB) is a low-level representation
that is useful internally, for mixing colours and performing computations
within the 3D lighting model.

The actual colour space that gives meaning to these coordinates
is defined by the rendering environment.
You should use high level operations to construct colour values,
and not try to create linear R,G,B triples by hand.

Future Work
-----------
* Add the LAB and HCL colour spaces.
  These are perceptually uniform colour spaces, useful for interpolation
  and generating colour sequences.
* Add sRGB.HWB colour space. It's the best RGB based colour space for colour picking by artists.
