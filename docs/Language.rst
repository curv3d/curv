=================
The Curv Language
=================

Curv is a domain specific language for specifying geometric objects.
It's a language for artists, and it prioritizes simplicity and exploratory
programming over the complex features provided by general purpose programming languages
to support large scale software engineering.

Curv is a dynamically typed pure functional language.
It has simpler semantics than any general purpose programming language.
It's powerful enough that the entire geometry API is written in Curv, not C++.
The core language is small enough to learn in a day.

1. Overview
===========

Values
------
In Curv, everything is a value.
All values are immutable (there are no mutable objects).
All data structures are hierarchical and finite, and therefore printable.
This design make Curv programs easier to understand and debug.

Curv has 7 primitive value types:

==============     ============================================
null               ``null``                
boolean            ``true, false``
number             ``-1, 3.1416, 0xFF``
string             ``"hello, world"``
list               ``[0,1,2]``
record             ``{name: "Lancelot", quest: "To seek the holy grail", favourite_colour: "blue"}``
function           ``x -> x+1``
==============     ============================================

The first 6 data types are taken from JSON,
and Curv can be considered a superset of JSON.

There are no type declarations or class declarations.
All application data is represented using the 7 value types.
For example, a geometric point or vector is a list of numbers.
A matrix is a list of lists of numbers. A geometric shape is a record
containing 5 fields (two booleans, two functions and a matrix).

Program Structure
-----------------
Curv has no I/O: it is not a scripting language.
The only goal of a Curv program is to compute a value:
usually this is a geometric shape, but it can also be a library.
(Shapes and libraries are both represented by record values.)

Conventional programming languages are "statement languages". A statement
is a syntactic form that either defines a named object (a definition)
or causes a side effect (an action). In such a language, a program is a sequence
of statements, the body of a function is a sequence of statements (one of which
is a "return statement"), and expressions occur only as elements of statements.

Curv is an "expression language". An expression is a syntactic form that
computes a value, eg like ``2+2``. A program is an expression. The body of a function
is an expression. Statements only occur embedded in certain expression forms.
(Note that JSON is also an expression language, and that most JSON programs are
also legal Curv programs.)

Blocks
------

Modules
-------

Functions
---------

List, Record and String Comprehensions
--------------------------------------

Design by Contract
------------------

Arrays and Vectorized Operations
--------------------------------

2. Values and Operations
========================

Null
----
``null`` is a placeholder value that, by convention, indicates that
some situation does not hold, and that the corresponding value is not available.
Its distinguishing characteristic
is that it is different from any other value. The only available
operation is testing a value to see if it is null: ``value==null``
is true if the value is null, otherwise false.

Boolean Values
--------------
The Boolean values are ``true`` and ``false``.
They are used for making decisions:
if some condition holds, do this, otherwise do that.

The relational operators compare two values and return a boolean:

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

The logical operators take boolean values as arguments, and return a boolean:

==========   =============================================================
``a && b``   Logical and: True if ``a`` and ``b`` are both true.
``a || b``   Logical or: True if at least one of ``a`` and ``b`` are true.
``!a``       Logical not: ``!true==false`` and ``!false==true``.
==========   =============================================================

The conditional operator selects between two alternatives based on a boolean condition::

  if (condition) result_if_true else result_if_false

For some algorithms, it is convenient to represent booleans as integers:
``true`` is ``1`` and ``false`` is ``0``. We support this via conversions
between boolean and integer:

* ``bit b`` -- convert boolean value ``b`` to an integer
* ``i != 0`` -- convert an integer bit value ``i`` to a boolean

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

Lists
-----
A list is a finite, ordered sequence of values.

Simple list constructors
  ``[]``, ``[a]``, ``[a,b]``, ``[a,b,c]``, ...
    A sequence of *n* comma separated expressions, enclosed in square brackets,
    constructs a list with *n* values.
  
  ``[a,]``, ``[a,b,]``, ``[a,b,c,]``, ...
    The final expression in a list constructor may have an optional trailing comma.
  
  ``()``, ``(a,b)``, ``(a,b,c)``, ...
    As an alternate syntax, you can also use parentheses to enclose the list,
    with the limitation that the list constructor must either be empty, or must contain
    commas.

  ``(a,)``, ``(a,b,)``, ``(a,b,c,)``, ...
    Ditto.

Range constructors
  ``i .. j``
    Returns the ascending list of numbers: ``i``, ``i+1``, ``i+2``, ... up to ``j`` inclusive.
    For example, ``1..10`` is ``[1,2,3,4,5,6,7,8,9,10]``.

  ``i .. j by k``
    Similar to ``i..j``, except that the step value is ``k`` instead of ``1``.
    The step value may be positive or negative, and need not be an integer.
    For example, ``1..0 by -0.25`` is ``[1, 0.75, 0.5, 0.25, 0]``.

  ``i ..< j``
    Same as ``i..j`` except that the final element in the sequence is omitted.
  
  ``i ..< j by k``
    Same as ``i..j by k`` except that the final element in the sequence is omitted.

List comprehensions
  The expressions in a simple list constructor are a special case of *element generators*.
  An element generator is a phrase that specifies a sequence of zero or more values.
  You can use iteration, conditionals, and local variables.
  A list constructor that uses the more general syntax is called a list comprehension.
  
  *expression*
    An expression is an element generator that generates a single value.
    
  ``if (condition) element_generator``
    The element generator is executed if the condition is true.
    
  ``if (condition) element_generator1 else element_generator2``
    A two armed conditional.
    
  ``for (pattern in list_expression) element_generator``
    The element_generator is executed once for each element in the list.
    At each iteration, the pattern is matched against a list element,
    and local variables specified by the pattern are bound.
    
  ``let definitions in element_generator``
    Introduce local variable bindings in an element generator.
    
  ``(eg1; eg2; eg3; ...)``
    A sequence of element generators separated by semicolons is a compound
    element generator. Each generator is executed in sequence.
    
  ``... list_expression``
    The spread operator (``...``) evaluates its argument expression, which must be a list,
    and outputs each element of the list in sequence.

  For example, ``[for (i in 1..10) i^2]`` yields ``[1,4,9,16,25,36,49,64,81,100]``.

List Indexing
  ``a[i]``
    The i'th element of list ``a``, if ``i`` is an integer.
    Zero based indexing: ``a[0]`` is the first element.

List Slicing
  ``a[indices]``
    Returns ``[a[indices[0]], a[indices[1]], ...]``,
    where ``indices`` is a list of integers.
    For example, ``a[0..<3]`` returns a list of the first 3 elements of ``a``.

``count a``
  The number of elements in list ``a``.

``concat aa``
  This is the list concatenation operator.
  ``aa`` is a list of lists. The component lists are catenated.
  For example, ``concat([1,2],[3,4])`` is ``[1,2,3,4]``.
  ``concat[]`` is ``[]``, because ``[]`` is the
  identity element for the concatenation operation.

``reverse a``
  A list containing the elements of ``a`` in reverse order.

``map f a``
  The list obtained by applying ``f`` to each element of list ``a``.
  Equivalent to ``[for (x in a) f x]``.

``filter p a``
  The list of those elements of ``a`` that satisfy the predicate ``p``.
  Equivalent to ``[for (x in a) if (p x) x]``.

``reduce (zero, f) a``
  Using binary function ``f``,
  iteratively combine all of the elements of list ``a`` into a single value,
  recursing on the left.
  For a 4 element list ``[a,b,c,d]``, this will compute::

    f(f(f(a,b),c),d)

  If the list has zero length, the result is ``zero``.

Tensors
-------
A **tensor** of rank *k* is a *k*-dimensional array of numbers.

The **rank** of a tensor is the number of integer indices that are required
to retrieve an element. If ``a`` is a tensor of rank 3, then ``a[i,j,k]``
is an element of ``a``.

Tensors are rectangular arrays.
The **dimensions** of a tensor of rank *k* is a *k*-element vector containing
the number of elements along each dimension.

* A number is a tensor of rank ``0`` and dimensions ``[]``.
* A vector with ``n`` elements is a tensor of rank 1 and dimensions ``[n]``.
  A vector is represented by a list of numbers.
* An *m* × *n* matrix is a tensor of rank 2 and dimensions ``[m,n]``.
  A matrix is represented by a list of *m* vectors of dimensions ``[n]``.
* In general, a tensor of rank ``n+1`` is represented by a list
  of tensors of rank ``n``, all with the same dimensions.

For example, here is the Curv representation of a 3x3 identity matrix::
  
    [[1,0,0],
     [0,1,0],
     [0,0,1]]

Note: The ``rank`` and ``dimensions`` operations are not implemented.

Indexing a Tensor
~~~~~~~~~~~~~~~~~
Tensors are indexed using a generalization of list indexing notation.
A tensor of rank *k* is indexed by a vector of count *k*.

For example, if ``M`` is a matrix, then ``M[i,j]`` retrieves
the element at row ``i`` and column ``j``, assuming ``i`` and ``j`` are integers.
This is just function call notation, where ``M`` plays the role of a function,
and the vector ``[i,j]`` plays the role of an argument.
``M[i,j]`` is equivalent to ``M[i][j]``, due to the representation of matrices
as nested lists.

Note: In theory, since ``42`` is a tensor of rank 0, it should be the case that
``42[] == 42``. However, this is not implemented.

Note: Tensor slicing is not implemented (yet).

Tensorized Numeric Operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
All of the built-in scalar numeric operations are generalized to operate on tensors.

Unary operations (like unary ``-``) are extended to operate "elementwise" on each
element of a tensor.  For example::

  -[[1,2],[3,4]] == [[-1,-2],[-3,-4]]

Binary operations (like binary ``+``) are extended in two ways:

* Elementwise: If the arguments are two lists of the same count, eg ``[a,b]+[c,d]``,
  then the result is another list of the same count, eg ``[a+c,b+d]``.
* Broadcasting: If one argument is a scalar, and the other argument is a list,
  eg ``x+[a,b]`` or ``[a,b]+x``,
  then you get ``[x+a,x+b]`` or ``[a+x,a+b]``.

These two rules are applied recursively in the case of nested lists.

N-ary operations like ``max``, which operate on a uniform list of arguments,
are treated as nested applications of a binary operator.
For example, ``max[a,b,c,d]`` is treated as ``max(max(max(a,b),c),d)``.

Tensorized numeric operations obey almost all of the same algebraic laws
as their scalar versions. For example, tensorized multiplication is
commutative, associative, and has an identity element of ``1``::

  [a,b]*[c,d] == [a*c,b*d] == [c*a,d*b] == [c,d]*[a,b]   // commutative
  1*[a,b] == [1*a,1*b] == [a,b]                          // identity element

Note: For matrix arguments, ``a*b`` performs elementwise (Hadamard) multiplication,
and not matrix multiplication. This is because it would be bad design to overload the same
symbol with two operations that obey different algebraic laws.
For standard matrix multiplication, use ``dot``, the tensor dot product.

Other Tensor Operations
~~~~~~~~~~~~~~~~~~~~~~~

``dot(a,b)``
  The tensor dot product ``a⋅b`` is a generalization of vector dot product and matrix multiplication.
  If V is a vector and M is a matrix, then:
  
  * ``dot(V1, V2)`` is the dot product of two vectors.
    Same as ``sum(V1 * V2)``, or ``V1*V2`` in OpenSCAD.
  * ``dot(V, M)`` is the product of a vector and a matrix.
    It's like matrix multiply, treating V as a column vector,
    but the result is a vector.
    Same as ``sum(V * M)``, or ``V*M`` in OpenSCAD.
  * ``dot(M, V)`` is the product of a vector and a matrix.
    It's like matrix multiply, treating V as a row vector, but the result is a vector.
    Same as ``sum(transpose M * V)``, or ``M*V`` in OpenSCAD.
  * ``dot(M1, M2)`` is standard matrix multiplication (``M1*M2`` in OpenSCAD).

  ``dot(a,b)`` works on any two tensors of at least rank 1,
  as long as the final dimension of ``a`` equals the first dimension of ``b``.
  The resulting tensor has rank equal to ``rank(a)+rank(b)-2``.
  This operation is equivalent to the ``Dot`` function in Mathematica,
  or to the following Curv definition::
  
    dot(a,b) =
      if (count a > 0 && is_list(a[0]))
        [for (row in a) dot(row,b)]  // matrix*...
      else
        sum(a*b)                     // vector*...

Points and Vectors
------------------
Geometric points and vectors are represented by a list of numbers.

Vec2 and Vec3
  ``(x,y)`` and ``(x,y,z)`` represent 2 and 3 dimensional points,
  and also 2 and 3 dimensional vectors.
  
  ``is_vec2 x``
    True if ``x`` is a 2 dimensional vector or point.
  
  ``is_vec3 x``
    True if ``x`` is a 3 dimensional vector or point.
  
  ``p[X]``, ``p[Y]``, ``p[Z]``
    The X and Y components of a Vec2 or Vec3.
    The Z component of a Vec3.
    Note: ``X=0``, ``Y=1`` and ``Z=2``.

``mag v``
  The magnitude of a vector ``v`` (sometimes called the length or the Euclidean norm).
  Equivalent to ``sqrt(sum(v^2))``.

``normalize v``
  Normalize a vector: convert it to a unit vector with the same direction.
  Equivalent to ``v / mag v``.

``dot(v1,v2)``
  Vector dot product (a special case of the tensor dot product).
  The result is a number. Same as ``sum(v1*v2)``.
  
  * Equivalent to ``mag v1`` × ``mag v2`` × *cos* θ,
    where θ is the angle between v1 and v2 and 0° ≤ θ ≤ 180°.
  * If v1 and v2 are at right angles, dot(v1,v2) == 0.
    If the angle is < 90°, the result is positive.
    If the angle is > 90°, the result is negative.
  * If v1 and v2 are unit vectors, then acos(dot(v1,v2)) is the angle
    between the vectors, in the range [0,tau/2].
    Note that this is expensive, and inaccurate for small angles.
    See ``perp`` for an alternative.
  * The scalar projection (or scalar component) of vector a in the direction of unit vector b
    is ``dot(a,b)``.
  
``phase v``
  The phase angle of a 2D vector, in the range ``tau/2`` to ``-tau/2``.
  This is the angle that the vector makes with the positive X axis,
  measured counter-clockwise.

``cis theta``
  Convert a phase angle to a unit 2D vector.

``perp v``
  Rotate a 2D vector by 90 degrees (tau/4) counterclockwise.
  Multiply a complex number by ``i``.
  ``perp(x,y)`` is equivalent to ``(-y,x)``.

  ``dot(perp v1, v2)`` is the "perp-dot product" of 2D vectors:

  * Equivalent to ``mag v1`` × ``mag v2`` × *sin* θ,
    where θ is the angle between v1 and v2, and -90° ≤ θ ≤ 90°.
    A positive angle indicates a counterclockwise rotation from v1 to v2.
  * Result is 0 if the vectors are colinear, >0 if the smaller angle from v1 to v2
    is a counterclockwise rotation, or <0 for a clockwise rotation.
  * The absolute value is the area of the parallelogram with the vectors as two sides,
    and so twice the area of the triangle formed by the two vectors.
  * ``z=(dot(a,b),dot(perp a,b))`` is a complex number that represents the signed angle
    between the vectors. ``phase z`` is the signed angle.
    ``cmul(pt, normalize z)`` rotates a point around the origin through that angle.
  * See 'The Pleasures of "Perp-Dot" Products', Graphics Gems IV.
  

``cross(v1,v2)``
  `Cross product`_ of 3D vectors, which is a 3D vector.
  
  * If v1 and v2 are colinear, or if either is (0,0,0), then the result is (0,0,0).
  * Otherwise, the result is a vector that is perpendicular to both v1 and v2
    and thus normal to the plane containing them.
  * The magnitude of the cross product equals the area of a parallelogram
    with the vectors for sides. For two perpendicular vectors, this is the product of their lengths.
  
.. _`Cross product`: https://en.wikipedia.org/wiki/Cross_product

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

Matrices
--------
``identity n``
  An ``n`` × ``n`` identity matrix.

``transpose a``
  The transpose of a matrix: a new matrix whose rows are the columns of the original.

``dot(a,b)``
  Matrix multiplication, a special case of the tensor dot product.
  If V is a vector and M is a matrix, then:
  
  * ``dot(V, M)`` is the product of a vector and a matrix.
    It's like matrix multiply, treating V as a column vector,
    but the result is a vector.
    Same as ``sum(V * M)``, or ``V*M`` in OpenSCAD.
  * ``dot(M, V)`` is the product of a vector and a matrix.
    It's like matrix multiply, treating V as a row vector, but the result is a vector.
    Same as ``sum(transpose M * V)``, or ``M*V`` in OpenSCAD.
  * ``dot(M1, M2)`` is standard matrix multiplication (``M1*M2`` in OpenSCAD).

Strings
-------
A string is a sequence of characters.
For example, ``"Hello, world"`` is a string literal.

Currently, only ASCII characters are supported (excluding the NUL character).
Later, Unicode will be supported.

``str[i]``
  String indexing: yields a string consisting of the i'th character of ``str``, with 0-based indexing.
  Individual characters are represented by strings, there is no separate 'character' data type.

``str[indices]``
  String slicing. ``indices`` is a list of character indexes.
  A new string is returned containing each indexed character from ``str``.
  For example, ``"foobar"[0..<3]`` yields ``"foo"``.

``count str``
  The number of characters in the string ``str``.

``repr x``
  Convert an arbitrary value ``x`` to a string.
  This is done by constructing an expression that, when evaluated, reproduces the
  original value. The exception to this is function values: currently ``repr``
  converts functions to the string ``"<function>"``.

``strcat list``
  String concatenation. ``list`` is a list of arbitrary values.
  Each element of ``list`` that isn't already a string is converted to a string using ``repr``.
  Then the resulting list of strings is concatenated into a single string.

``encode str``
  Convert a string to a list of Unicode code points (represented as integers).

``decode codelist``
  Convert a list of Unicode code points to a string.
  Currently the only supported code points are 1 to 127.

``nl``
  A string containing a single newline character.
  It is intended for substitution into a string literal.

String Constructor
  A string constructor is enclosed in double-quotes (``"`` characters)
  and contains a sequence of zero or more segments.
  
  * A literal segment is a sequence of ASCII characters,
    excluding the characters NUL, ``"`` and ``$``,
    which are added to the string under construction with no interpretation.
  * ``""`` is interpreted as a single ``"`` character.
  * ``$$`` is interpreted as a single ``$`` character.
  * ``${expr}`` interpolates the value of ``expr``. If the value is not a string,
    then it is converted to a string by ``repr``.
  * ``$(expr)`` is equivalent to ``${repr(expr)}``.
  * ``$[...]`` is equivalent to ``${decode[...]}``.
  * ``$identifier`` is equivalent to ``${identifier}``.

Records
-------
A record is a set of fields (name/value pairs).
If ``R`` is the record value ``{a:1,b:2}``,
then ``R.a`` is ``1``, the value of field ``a``.

Records are used to represent:

* sets of labeled arguments in a function call;
* geometric shapes (each field is a shape attribute);
* modules or libraries.

Record Constructors
~~~~~~~~~~~~~~~~~~~

A record constructor consists of a comma-separated list of field specifiers
inside of brace brackets. A field specifier has one of these forms:

* *identifier* ``:`` *expression*
* *quotedString* ``:`` *expression*
* ``...`` *recordExpression*

Field names can be arbitrary strings, but as an abbreviation, they can be
specified as just bare identifiers.
The spread operator (``...``) interpolates all of the fields from another
record into the record being constructed.

Field specifiers are processed left to right. If the same field name is
used more than once, then the last field specifier wins.

Examples:

* ``{a: 1, b: 2}``
* ``{"a": 1, "b": 2}`` -- Same as above.
* ``{x: 0, ... r}`` -- The same as record ``r``, except that if field ``x`` is
  not defined, then it defaults to ``0``.
* ``{... r, x: 0}`` -- The same as record ``r``, extended with field ``x``,
  with ``x:0`` overriding any previous binding.

Compound field specifiers may be constructed using blocks, and using the
``if``, ``for`` and ``;`` control structures, as described later.

Record Operations
~~~~~~~~~~~~~~~~~
``record . identifier``
  The value of the field named by ``identifier``.
  Eg, ``r.foo``.

``record . quotedString``
  The value of the field named by ``quotedString``.
  Eg, ``r."foo"``.
  The field name need not be a constant. Eg, ``r."$x"``.

``defined (record . identifier)``
  True if a field named ``identifier`` is defined by ``record``, otherwise false.

``defined (record . quotedString)``
  True if a field named ``quotedString`` is defined by ``record``, otherwise false.

``fields record``
  The field names defined by ``record`` (as a list of strings).

``merge listOfRecords``
  Merge all of the fields defined by the records in ``listOfRecords``
  into a single record. If the same field is defined more than once,
  the last occurrence of the field wins.
  Same as::

    {for (r in listOfRecords) ...r}

Functions and Patterns
----------------------
function constructor

function definition

pattern

function call

switch

Debug Actions
-------------
Curv programs are debugged by inserting ``print`` statements and other actions.

An action is a phrase that has a debug-related side effect, but which
does not compute a value.

``do action in phrase``
  Execute the ``action``, then evaluate the ``phrase``.

  The ``action`` argument can be a simple action, as enumerated below,
  or it can be a compound action which is sequenced using ``if``, ``for`` and ``;``
  or which defines local variables using ``let`` and ``where``.
  For example, you can write a sequence of actions separated by ``;``,
  and they will be executed in sequence.

  The ``phrase`` argument can be an expression or statement.
  (A statement is an element specifier in a list constructor,
  a field specifier in a record constructor, or an action.)
  A ``do`` phrase can be used in any context where its ``phrase`` argument
  would also be legal.

  For example::

    f x =
      do print "calling function f with argument x=$x";
      in x+1;

  Then ``f 2`` returns ``3``, and as a side effect, prints a message
  to the debug console.

Simple Actions
~~~~~~~~~~~~~~

``print message``
  Print a message string on the debug console, followed by newline.
  If ``message`` is not a string, it is converted to a string using ``repr``.

``warning message``
  Print a message string on the debug console, preceded by "WARNING: ",
  followed by newline and then a stack trace.
  If ``message`` is not a string, it is converted to a string using ``repr``.

``error message``
  On the debug console, print "ERROR: ", then the message string,
  then newline and a stack trace. Then terminate the program.
  If ``message`` is not a string, it is converted to a string using ``repr``.
  An error phrase is legal in either an action context, or in an expression context.

``assert condition``
  Evaluate the condition, which must be true or false.
  If it is true, then nothing happens.
  If it is false, then an assertion failure error message is produced,
  followed by a stack trace, and the program is terminated.

``assert_error(error_message_string, expression)``
  Evaluate the expression argument.
  Assert that the expression evaluation terminates with an error,
  and that the resulting error message is equal to ``error_message_string``.
  Used for unit testing.

``exec expression``
  Evaluate the expression and then ignore the result.
  This is used for calling a function whose only purpose is to have a side effect
  (by executing debug actions) and you don't care about the result.

Blocks
------
Any expression or phrase may have local variables.

* ``let definitions in phrase``
* ``phrase where definitions``

Control Structures: ``if``, ``for`` and ``;``
---------------------------------------------

Patterns
--------

Design by Contract
------------------
::

  is_null
  is_bool
  is_num
  is_string
  is_list
  is_record
  is_fun

  ensure pred expr = do assert(pred expr) in expr;
  assert
  error

Source Files and External Libraries
-----------------------------------
::

  file
  include record_expr

3. Grammar
==========

The Surface Grammar
-------------------
The surface grammar is a simplified grammar that describes the hierarchical
structure of Curv programs, but doesn't ascribe meaning to parse tree nodes.
Not all programs that have a parse tree are syntactically correct.

There are 12 operator precedence levels, with ``list`` being the lowest precedence
and ``postfix`` being the highest precedence::

  program ::= list

  list ::= empty | item | commas | semicolons | item 'where' list
    commas ::= item ',' | item ',' item | item ',' commas
    semicolons ::= optitem | semicolons `;` optitem
    optitem ::= empty | item

  item ::= pipeline
    | '...' item
    | 'include' item
    | pipeline '=' item
    | pipeline ':=' item
    | pipeline ':'
    | pipeline ':' item
    | pipeline '->' item
    | pipeline '<<' item
    | 'if' parens item
    | 'if' parens item 'else' item
    | 'for' '(' item 'in' item ')' item
    | 'while' parens item
    | 'let' list 'in' item
    | 'do' list 'in' item

  pipeline ::= disjunction
    | pipeline '>>' disjunction
    | pipeline '`' postfix '`' disjunction

  disjunction ::= conjunction | disjunction '||' conjunction

  conjunction ::= relation | conjunction && relation

  relation ::= range
    | range '==' range | range '!=' range
    | range '<' range  | range '>' range
    | range '<=' range | range '>=' range

  range ::= sum
    | sum '..' sum
    | sum '..' sum 'by' sum
    | sum '..<' sum
    | sum '..<' sum 'by' sum

  sum ::= product | sum '+' product | sum '-' product

  product ::= unary | product '*' unary | product '/' unary

  unary ::= power | '-' unary | '+' unary | '!' unary | 'var' unary

  power ::= postfix | postfix '^' unary

  postfix ::= primary
    | postfix primary
    | postfix '.' primary

  primary ::= identifier | numeral | string | parens | brackets | braces
    identifier ::= /[a-zA-Z_] [a-zA-Z_0-9]*/, except for reserved words
      reserved_word ::= '_' | 'by' | 'do' | 'else' | 'for' | 'if'
        | 'in' | 'include' | 'let' | 'var' | 'where' | 'while'

    numeral ::= hexnum | mantissa | /mantissa [eE] [+-]? digits/
      mantissa ::= /digits/ | /'.' digits/ | /digits '.'/ | /digits '.' digits/
      digits ::= /[0-9]+/
      hexnum ::= /'0x' [0-9a-fA-F]+/

    string ::= /'"' segment* '"'/
      segment ::= /[white space or printable ASCII character, except for " or $]+/
        | /'""'/
        | /'$$'/
        | /'${' list '}'/
        | /'$[' list ']'/
        | /'$(' list ')'/
        | /'$' identifier/

    parens ::= '(' list ')'
    brackets ::= '[' list ']'
    braces ::= '{' list '}'

  C style comments, either '//' to end of line, or '/*'...'*/'

The Deep Grammar: Phrases
-------------------------
There is a deeper phrase-structure grammar that assigns syntactic meanings
to most parse tree nodes, which are now called phrases.
(Some parse tree nodes do not have an independent meaning, and are not phrases.)
There are 6 phrase types:

definition
  A phrase that binds zero or more names to values, within a scope.

pattern
  A pattern can occur as a function formal parameter,
  or as the left side of a definition, and contains usually one
  (but generally zero or more) parameter names.
  During pattern matching,
  we attempt to match an argument value against a pattern.
  If the match is successful, we bind (each) parameter name
  to (elements of) the argument value.

expression
  A phrase that computes a value.

action
  A phrase that causes a side effect, and doesn't compute a value.

element generator
  A phrase that computes a sequence of zero or more values.
  ``[``\ *element_generator*\ ``]`` is a list comprehension.

field generator
  A phrase that computes a sequence of zero or more fields,
  which are name/value or string/value pairs.
  ``{``\ *field_generator*\ ``}`` is a record comprehension.

An action can be used in any context requiring a definition,
element generator, or field generator. An expression can be used
in any context requiring a field generator.

Programs
--------
There are two kinds of programs.
A source file is always interpreted as an expression.
A command line (in the ``curv`` command line interpreter)
can be an expression, an action, or a definition.

Phrase Abstraction
------------------
Curv has a set of generic operations for constructing more complex phrases
out of simpler phrases. These operations work on multiple phrase types,
and support sequencing, conditional evaluation, iteration, and local variables.

Parenthesized phrase: ``(phrase)``
  Any phrase can be wrapped in parentheses without changing its meaning.

Compound phrase: ``phrase1; phrase2``
  * If both phrases are definitions, then this is a compound definition.
    The order doesn't matter, and the definitions may be mutually recursive.
  * If both phrases are actions, element generators, or field generators,
    then the phrases are executed in sequence.

Single-arm conditional: ``if (condition) phrase``
  If the phrase is an action, element generator, or field generator,
  then the phrase is only executed if the condition is true.

Double-arm conditional: ``if (condition) phrase1 else phrase2``
  The phrases may be expressions, actions, element generators, or field generators.

Bounded iteration: ``for (pattern in list_expression) phrase``
  The phrase may be an action, element generator, or field generator.
  The phrase is executed once for each element in the list.
  At each iteration,
  the element is bound to zero or more local variables by the pattern.

Local variables: ``let definition in phrase``
  Define local variables over the phrase.
  The phrase can be an expression, action, element generator or field generator.

Local variables: ``phrase where definition``
  An alternate syntax for defining local variables.

Local actions: ``do action in phrase``
  The phrase can be an expression, action, element generator or field generator.
  The action is executed first, then the phrase is evaluated.

4. The Imperative Sublanguage
=============================
Curv contains an "imperative sublanguage", implemented by the ``do`` operator,
which allows you to define mutable variables using ``var``,
reassign those variables using ``:=``, and iterate using
a ``while`` action. This allows you to write code in an imperative style.
The semantics of this feature are restricted, so that it is impossible to define
impure functions. Curv retains its pure functional semantics.

This feature exists for 3 reasons:

* Makes it easier to port code from an imperative language.
* It's an aid to users whose primary programming experience
  is with imperative languages, and who are not fully fluent
  in the functional programming style.
* In the 0.0 release, this is the only way to iterate within a shape's distance
  function. The GPU compiler is not yet smart enough to convert tail recursion
  into iteration.
