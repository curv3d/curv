Intensity Fields
================
An intensity field is a colour pattern with the colours abstracted away,
leaving only a pattern of intensity values, which are numbers between 0 and 1.
Intensity fields can be animated.

Mathematically, an intensity field is a function
that maps (x,y,z,t) onto an intensity,
where (x,y,z) is a point in space, and t is time.

An intensity field can be composed with a colour map
to construct a texture.

Operations
----------
This interface is still experimental.

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

Future Work
-----------
* Constructors, that build intensity fields.
* Transformations, that map one ifield onto another.
* Combinators, that combine two or more ifields, creating a new ifield.
* Fractal noise, used to create natural looking textures like wood and marble.
