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
  `Tau is the fundamental circle constant`_.
  You can use it to specify angles:
  
  ====== =======
  tau    360*deg
  tau/2  180*deg
  tau/3  120*deg
  tau/4   90*deg
  tau/6   60*deg
  tau/8   45*deg
  tau/12  30*deg
  ====== =======

.. _`Tau is the fundamental circle constant`: https://tauday.com/tau-manifesto

``sin a``
  The sine of angle ``a``.

``cos a``
  The cosine of angle ``a``.

``tan a``
  The tangent of angle ``a``.

``cis a``
  Convert a phase angle to a unit 2D vector (see ``phase``).
  This is ``[cos a, sin a]``.

``asin x``
  The principal value of the arc sine (inverse sine) of x.
  The result is an angle in the range [-tau/4, +tau/4].

``acos x``
  The principle value of the arc cosine (inverse cosine) of x.
  The result is an angle in the range [0, tau/2].

``atan x``
  The principal value of the arc tangent (inverse tangent) of x.
  The result is an angle in the range [-tau/4, +tau/4].
  The ``phase`` function is generally more useful.

``phase[x,y]``
  The phase angle of a 2D vector,
  which is the angle that the vector makes with the positive X axis, measured counter-clockwise.
  This is the principal value of the arc tangent of ``y/x``,
  using the signs of ``x`` and ``y`` to determine the quadrant of the return value.
  The result is in the range [-tau/2, +tau/2].

``sec a``
  The secant (reciprocal sine) of angle ``a``.

``csc a``
  The cosecant (reciprocal cosine) of angle ``a``.

``cot a``
  The cotangent (reciprocal tangent) of angle ``a``.

Hyperbolic Functions
--------------------

``sinh x``
  The hyperbolic sine of ``x``.

``cosh x``
  The hyperbolic cosine of ``x``.

``tanh x``
  The hyperbolic tangent of ``x``.

``asinh x``
  The inverse hyperbolic sine of ``x``.

``acosh x``
  The principal value of the inverse hyperbolic cosine of ``x``.

``atanh x``
  The inverse hyperbolic tangent of ``x``.
