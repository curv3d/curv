Numbers
-------
Numbers are represented internally by 64 bit IEEE floating point numbers.

* ``0`` and ``-0`` are different numbers, but they compare equal.
* ``inf`` is infinity; it is greater than any finite number.
* ``-inf`` is negative infinity.
* We don't support the IEEE NaN (not a number) value.
  Instead, all arithmetic operations with an undefined result report an error.
  For example, ``0/0`` is an error.
* An integer is just a number whose fractional part is ``0``.
  ``1`` and ``1.0`` evaluate to the same integer.

``is_num value``
  True if the value is a number, false otherwise.

The arithmetic operators:

=========  ==============
``-m``     negation
``+m``     identity
``m + n``  addition
``m - n``  subtraction
``m * n``  multiplication
``m / n``  division
``m ^ n``  exponentiation
=========  ==============

``abs n``
  The absolute value of *n*.

``floor n``
  The largest integer less than or equal to *n*.

``ceil n``
  The smallest integer greater than or equal to *n* (ceiling).

``trunc n``
  The integer nearest to but no larger in magnitude than *n* (truncate).

``round n``
  The integer nearest to *n*. In case of a tie (the fractional part of *n* is 0.5),
  then the result is the nearest even integer.

``frac n``
  The fractional part of *n*.
  Equivalent to ``n - floor n``, or to ``mod(n,1)``.
  The function ``y=frac x`` is a sawtooth wave.

``max list``
  The maximum value in a list of numbers.
  ``max[]`` is ``-inf``, which is the identity element for the maximum operation.

``min list``
  The minimum value in a list of numbers.
  ``min[]`` is ``inf``, which is the identity element for the minimum operation.

``sum list``
  The sum of a list of numbers.
  ``sum[]`` is ``0``, which is the identity element for the sum operation.

``product list``
  The product of a list of numbers.
  ``product[]`` is ``1``, which is the identity element for the product operation.

``sort list``
  Sort a list of numbers into ascending order.

``mod(a,m)``
  The remainder after dividing ``a`` by ``m``,
  where the result has the same sign as ``m``.
  Equivalent to ``a - m * floor(a/m)``.
  Aborts if ``m==0``.

``rem(a,m)``
  The remainder after dividing ``a`` by ``m``,
  where the result has the same sign as ``a``.
  Equivalent to ``a - m * trunc(a/m)``.
  Aborts if ``m==0``.

``phi``
  The golden ratio: ``(sqrt 5 + 1) / 2 == 1.618033988749895``.

``e``
  Euler's number: ``2.718281828459045``.

``sqrt n``
  Square root of *n*.

``log n``
  Natural logarithm (to the base *e*) of *n*.

``clamp(n,lo,hi)``
  Constrain ``n`` to lie between ``lo`` and ``hi``.
  Equivalent to ``min(max(n,lo),hi)``.

``lerp(lo,hi,t)``
  Linear interpolation between ``lo`` and ``hi``
  using parameter ``t`` as a weight: ``t==0`` returns ``lo``
  and ``t==1`` returns ``hi``.
  Equivalent to ``lo*(1-t)+hi*t``.

``smoothstep(lo,hi,x)``
  Return 0 if x <= lo; 1 if x >= hi;
  otherwise smoothly interpolate between 0 and 1 using a Hermite polynomial.
  Result is undefined if lo >= hi.
  https://en.wikipedia.org/wiki/Smoothstep
