Trigonometry
------------
Curv uses `radians`_ (not degrees) to specify angles.

.. _`radians`: https://en.wikipedia.org/wiki/Radian

``pi``
  The ratio of a circle's circumference to its diameter: ``3.141592653589793``.

``deg``
  The number of radians in an angle of 1 degree.
  To specify the angle "45 degrees", use ``45*deg``.

``tau``
  The number of radians in a full turn, aka ``2*pi`` or ``360*deg``.
  
  ====== =======
  tau    360*deg
  tau/2  180*deg
  tau/3  120*deg
  tau/4   90*deg
  tau/6   60*deg
  tau/8   45*deg
  tau/12  30*deg
  ====== =======

``sin x``
  The sine of ``x``, measured in radians.

``cos x``
  The cosine of ``x``, measured in radians.

``tan x``
  The tangent of ``x``, measured in radians.

``asin x``
  The principal value of the arc sine (inverse sine) of x.
  The result is in the range [-tau/4, +tau/4].

``acos x``
  The principle value of the arc cosine (inverse cosine) of x.
  The result is in the range [0, tau/2].

``atan x``
  The principal value of the arc tangent (inverse tangent) of x.
  The result is in the range [-tau/4, +tau/4].
  The ``atan2`` function is generally more useful.

``atan2(y,x)``
  The principal value of the arc tangent of y/x,
  using the signs of both arguments to determine the quadrant of the return value.
  The result is in the range [-tau/2, +tau/2].
  
  Used mostly to convert from rectangular to polar coordinates,
  but for that, you should consider using ``phase(x,y)`` instead,
  so that you don't have to flip the x,y coordinates.

``sec x``
  The secant (reciprocal sine) of x.

``csc x``
  The cosecant (reciprocal cosine) of x.

``cot a``
  The cotangent (reciprocal tangent) of x.
