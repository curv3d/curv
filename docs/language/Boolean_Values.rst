Boolean Values
--------------
The Boolean values are ``#true`` and ``#false``.
They are used for making decisions:
if some condition holds, do this, otherwise do that.

Booleans are a special case of `Symbols`_.

.. _`Symbols`: Variant.rst

The relational operators compare two values and return a Boolean:

==============     ============================================
``a == b``         ``a`` is equal to ``b``
``a != b``         ``a`` is not equal to ``b``
``m < n``          ``m`` is less than ``n``
``m <= n``         ``m`` is less than or equal to ``n``
``m > n``          ``m`` is greater than ``n``
``m >= n``         ``m`` is greater than or equal to ``n``
==============     ============================================

(Note: ``a`` and ``b`` are arbitrary values, ``m`` and ``n`` are numbers.)

The ``==`` operator is an equivalence relation:

* For every value ``a``, ``a == a``.
* For any pair of values ``a`` and ``b``, if ``a==b`` then ``b==a``.
* For any three values ``a``, ``b`` and ``c``, if ``a==b`` and ``b==c`` then ``a==c``.

The logical operators take Boolean values as arguments, and return a Boolean:

==========   =============================================================
``a && b``   Logical and: True if ``a`` and ``b`` are both true.
``a || b``   Logical or: True if at least one of ``a`` and ``b`` are true.
``!a``       Logical not: ``!true==false`` and ``!false==true``.
==========   =============================================================

The conditional operator selects between two alternatives based on a Boolean condition::

  if (condition) result_if_true else result_if_false

For some algorithms, it is convenient to represent Booleans as integers:
``#true`` is ``1`` and ``#false`` is ``0``. We support this via conversions
between Boolean and integer:

* ``bit b`` -- convert Boolean value ``b`` to an integer
* ``i != 0`` -- convert an integer bit value ``i`` to a Boolean

``is_bool value``
  True if the value is Boolean, false otherwise.
