Colour Maps
===========

A *colour map* is a function that maps an intensity (a number between 0 and 1)
onto a colour.

Colour maps are used for constructing procedural textures.

Cyclic Colour Maps
------------------
Cyclic colour maps have colours that are matched at each end.

``sRGB.hue``
  A "rainbow" colour map, containing all of the pure hues in the sRGB colour space.
  0 is red, 1/3 is green, 2/3 is blue, and so on.
  Not perceptually uniform: there are bright bands at yellow, cyan and magenta.

Linear Colour Maps
------------------
Linear colour maps have colour lightness values that increase or decrease linearly.

``grey_cmap``
  A greyscale colour map, a continuous gradient from 0 (black) to 1 (white).

Utilities
---------
``gradient_cmap clist``
  Construct a colour map as a sequence of one or more colour gradients.
  The ``clist`` is a list of two or more entries.
  Each entry is ``(intensity,colour)``, ordered with monotonically increasing intensity values.
  The first intensity must be ``0``, and the last must be ``1``.
  A gradient is constructed between each pair of entries.

``striped_cmap clist``
  Construct a colour map as a sequence of constant-colour stripes.
  The ``clist`` argument is the same as for ``gradient_cmap``,
  except that the final entry has an intensity of < 1,
  and one stripe is constructed for each entry.
  
``show_cmap f``
  Construct a visualization of the colour map f (returns a shape).
