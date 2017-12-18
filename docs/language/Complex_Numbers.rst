Complex Numbers
---------------
A complex number is represented by the ordered pair ``(re,im)``.
Complex numbers are interpreted geometrically: they are indistinguishable
from 2D points/vectors, and they share the same set of operations.

``(re, im)``
  Construct a complex number with real part ``re``
  and imaginary part ``im``.

``z[RE]``, ``z[IM]``
  The real and imaginary components of a complex number.
  Note: ``RE=0`` and ``IM=1``.

``mag z``
  The *absolute value* (or *modulus* or *magnitude*) of a complex number.

``phase z``
  The *argument* (or *phase*) of a complex number:
  it is an angle in radians in the range [-tau/2,tau/2].

``r * cis theta``
  Construct a complex number from polar coordinates:
  the magnitude is ``r`` and the phase is ``theta``.

``-z``, ``z + w``, ``z - w``
  Add and subtract complex numbers.

``cmul(z, w)``
  Multiply two complex numbers.
  This multiplies the magnitudes and adds the phase angles of ``z`` and ``w``.
  If ``z`` is a 2D point and ``w`` is a unit vector with phase angle ``theta``,
  then ``cmul(z,w)`` rotates the point ``z`` around the origin by angle ``theta``.
  An appropriate value ``w`` may be obtained using ``cis theta``.
  So ``cmul`` is a 2D rotation operator.

``csqr z``
  Square a complex number.
