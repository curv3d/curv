Boolean Values
==============
The Boolean values are ``#true`` and ``#false``.
They are used for making decisions:
if some condition holds, do this, otherwise do that.

Booleans are a special case of `Symbols`_.

.. _`Symbols`: Variant.rst

Equality Operators
------------------
The equality operators compare two arbitrary values ``a`` and ``b``
for equality, and return a Boolean.

==============     ============================================
``a == b``         ``a`` is equal to ``b``
``a != b``         ``a`` is not equal to ``b``: ``not(a==b)``
==============     ============================================

The ``==`` operator is an equivalence relation:

* For every value ``a``, ``a == a``.
* For any pair of values ``a`` and ``b``, if ``a==b`` then ``b==a``.
* For any three values ``a``, ``b`` and ``c``, if ``a==b`` and ``b==c``
  then ``a==c``.

Order Operators
---------------
The order operators compare two numbers ``m`` and ``n``
for their relative ordering and return a Boolean:

==============     ============================================
``m < n``          ``m`` is less than ``n``
``m <= n``         ``m`` is less than or equal to ``n``
``m > n``          ``m`` is greater than ``n``
``m >= n``         ``m`` is greater than or equal to ``n``
==============     ============================================

Logic Operations
----------------
The logic functions ``not``, ``and``, ``or`` and ``xor``
take Boolean values as arguments, and return a Boolean.

``not a``
  True if ``a`` is false. False if ``a`` is true.

``and list``
  The logical conjunction of a list of booleans: true only if all of the
  booleans in the list are true, otherwise false.
  ``and[]`` is ``#true``, which is the identity element
  for the ``and`` operation.

``or list``
  The logical disjunction of a list of booleans: false only if all of the
  booleans in the list are false, otherwise true.
  ``or[]`` is ``#false``, which is the identity element
  for the ``or`` operation.

``xor list``
  The "exclusive or" of a list of booleans.
  In the case of two elements,
  ``xor[a,b]`` is true only if ``a`` and ``b`` are different.
  In the general case, ``xor list`` is true only if there are an odd
  number of list elements that are true.
  ``xor[]`` is ``#false``, which is the identity element
  for the ``xor`` operation.

There are two "short circuit" logic operators, ``&&`` for 'and',
and ``||`` for 'or', which do not evaluate the second argument,
if the result is determined by the first argument.

==========   =============================================================
``a && b``   Logical and: True if ``a`` and ``b`` are both true.
             Don't evaluate ``b`` if ``a`` is false.
----------   -------------------------------------------------------------
``a || b``   Logical or: True if at least one of ``a`` and ``b`` are true.
             Don't evaluate ``b`` if ``a`` is true.
==========   =============================================================

Conditional Operator
--------------------
The conditional operator selects between two alternatives based on a Boolean condition::

  if (condition) result_if_true else result_if_false

Bits
----
For some algorithms, it is convenient to represent Booleans as integers:
``#true`` is ``1`` and ``#false`` is ``0``. We support this via conversions
between Boolean and integer:

* ``bit b`` -- convert Boolean value ``b`` to an integer
* ``i != 0`` -- convert an integer bit value ``i`` to a Boolean

Predicates
----------
``is_bool value``
  True if the value is Boolean, false otherwise.

Vectorized Boolean Operations
-----------------------------
All of the Boolean operations are available in "vectorized" form.
These are generalized to operate on single Booleans, or lists of
Booleans, or lists of lists of Booleans, or in general, trees of Booleans.

The equality operators ``==`` and ``!=``
have vectorized equivalents, the ``equal`` and ``unequal`` functions.
Note that ``==`` will compare two lists for equality, and return a single
Boolean result, but you can't do that using ``equal``, because it
will compare list elements for equality, and return a list of Booleans.

The order operators ``<``, ``<=``, ``>`` and ``>=`` are vectorized:
if passed a list of numbers, instead of a single number, they will return
a list of Booleans. For example::

    [0,1] < [1,0]   =>   [#true,#false]

The logic functions ``not``, ``and``, ``or`` and ``xor`` are vectorized.

The conditional operator (``if``)
has a vectorized equivalent, the ``select`` function::

    select [condition, result_if_true, result_if_false]

The ``bit`` function is vectorized.
