Colour Maps
===========

A *colour map* is a function that maps an intensity (a number between 0 and 1)
onto a colour.

Colour maps are used for constructing textures.

Cyclic Colour Maps
------------------
Cyclic colour maps have colours that are matched at each end.

``sRGB.hue``
  A "rainbow" colour map, containing all of the pure hues in the sRGB colour space.
  0 is red, 1/3 is green, 2/3 is blue, and 1 is red.
  Not perceptually uniform: there are bright bands at yellow, cyan and magenta.

Linear Colour Maps
------------------
Linear colour maps have colour lightness values that increase or decrease linearly.

``sRGB.grey``
  A greyscale colour map, a continuous gradient from 0 (black) to 1 (white).

Utilities
---------
``show_cmap f``
  Construct a visualization of the colour map f (returns a shape).

Future Work
-----------
* A larger set of predefined colour maps.
* Parameterized constructors, that build families of colour maps.
* Transformations, that map one cmap onto another.
* Combinators, that combine two or more cmaps, creating a new cmap.
