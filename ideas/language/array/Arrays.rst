Arrays
======
Curv is an "array language", in the spirit of APL, K, Mathematica, Python's
numpy, Google's TensorFlow, and many others.

Most of Curv's primitive operations are "vectorized",
which means they are extended to operate on arrays.
For example, the binary `+` operator doesn't just add two numbers.
Due to being vectorized, it can also do this::

    2 + [1,2,3] == [3,4,5]
    [1,2,3] + [10,11,12] == [11,13,15]

Vectorization is important because:

 * It extends basic arithmetic operations to perform common linear algebra
   operations, which is useful for doing computer graphics.
 * CPUs and GPUs have hardware support for parallel vector operations.
   Vectorized operations in Curv map directly onto these hardware capabilities,
   which speeds up rendering.

Not all operations can be automatically vectorized.
To understand why, we need to understand the theory of arrays in Curv.

Ranked Arrays
-------------
A **ranked array** (or just *array* for short) is a *k*-dimensional array
of scalar values, represented in Curv by nested lists.
(These generalized *k*-dimensional arrays are called *tensors* in TensorFlow,
but almost every other array language uses the term "array".)

The **rank** of an array is the number of integer indices that are required
to retrieve an element. If ``a`` is an array of rank 3, then ``a[i,j,k]``
is an element of ``a``.

The **dimensions** of an array of rank *k* is a *k*-element list containing
the number of elements along each dimension.

* A scalar is a tensor of rank ``0`` and dimensions ``[]``.
  Note that a scalar is any Curv value that is not a list.
* A vector with ``n`` elements is an array of rank 1 and dimensions ``[n]``.
  A vector is represented by a non-empty list of scalars.
* An *m* × *n* matrix is an array of rank 2 and dimensions ``[m,n]``.
  A matrix is represented by a non-empty list of *m* vectors of dimensions ``[n]``.
* In general, an array of rank ``n+1`` is represented by a non-empty list
  of tensors of rank ``n``, all with the same dimensions.

You will note that all of the dimensions in a ranked array are non-zero.
The empty list ``[]`` does not have a rank and is not a ranked array.
This is because Curv uses nested lists to represent arrays. There is no way
in Curv to distinguish a vector of dimension ``[0]`` from a matrix of
dimension ``[0,0]``, so we simply do not support arrays with empty dimensions.

For example, here is the Curv representation of a 3x3 identity matrix::
  
    [[1,0,0],
     [0,1,0],
     [0,0,1]]

Note: The ``rank`` and ``dimensions`` operations are not implemented.

Vectorized Scalar Functions
---------------------------
All of Curv's primitive scalar operations are generalized to operate
on arrays and nested lists.

Unary scalar operations (like unary ``-``) are extended to operate "elementwise"
on each scalar element of an array or nested list.  For example::

  -[[1,2],[3,4]] == [[-1,-2],[-3,-4]]

Binary scalar operations (like binary ``+``) are extended in two ways:

* Elementwise: If the arguments are two lists of the same count,
  eg ``[a,b]+[c,d]``,
  then the result is another list of the same count, eg ``[a+c,b+d]``.
* Broadcasting: If one argument is a scalar, and the other argument is a list,
  eg ``x+[a,b]`` or ``[a,b]+x``,
  then you get ``[x+a,x+b]`` or ``[a+x,a+b]``.

These two rules are applied recursively in the case of nested lists.

Curv has a large number of scalar reduction operations, which map a list
of zero or more scalars to a single scalar, by repeatedly applying a
commutative binary operation. Examples include: ``sum``, ``product``,
``max``, ``min``, ``and``, and ``or``.

These scalar reduction operations are extended to operate on nested lists
using broadcasting and elementwise operation.
For example::

  sum[1,2,3] == 6
  sum[400,[10,20],[7,8]] == [417,428]

Vectorized numeric operations obey almost all of the same algebraic laws
as their scalar versions. For example, vectorized multiplication is
commutative, associative, and has an identity element of ``1``::

  [a,b]*[c,d] == [a*c,b*d] == [c*a,d*b] == [c,d]*[a,b]   // commutative
  1*[a,b] == [1*a,1*b] == [a,b]                          // identity element

Note: For matrix arguments, ``a*b`` performs elementwise (Hadamard)
multiplication, and not matrix multiplication. This is because it would
be bad design to overload the same symbol with two operations that obey
different algebraic laws. For standard matrix multiplication, use ``dot``,
the tensor dot product.

Vectorized General Functions
----------------------------
In the previous section, I described how to vectorize a scalar function.
Non-scalar functions can be vectorized,
as long as they can be assigned a *rank*.
Such functions can be called **ranked functions**.

* For a unary function, this means that all arguments in the function's domain
  are ranked arrays with the same rank. A scalar unary function is a special
  case of this: it is a ranked unary function with rank 0.
* For a binary function whose argument is a pair ``[x,y]``,
  a consistent rank must be assigned to the first element of the pair,
  and to the second element of the pair, but these two ranks do not need
  to be the same.
* For an n-ary reduction function, whose argument is a list of zero or more
  elements from some commutative binary operation, the element values must
  have a consistent rank.

Here are some examples of non-scalar ranked functions, which are vectorized
in Curv:

* ``phase`` -- rank 1 (arguments are 2-vectors)

Many functions do not have a rank, and cannot be vectorized.
* Unary predicate functions that take an arbitrary value as an argument,
  and return a boolean.
* Binary predicate functions like ``==`` and ``!=``, which operate on arbitrary
  values. However, the alternative equality functions ``equal`` and ``unequal``
  compare scalars and are vectorized.
* List operations like ``count``, ``concat``, ``sort``, ``reverse``.
  Also ``transpose``.
* ``dot``

Some functions cannot be vectorized for other reasons.
* The ``&&`` and ``||`` operators have short-circuit (lazy evaluation)
  semantics which would be destroyed by vectorization. So we instead
  provide the alternative vectorized reduction functions ``and`` and ``or``.
* Curried functions like ``map`` and ``filter``.
  At present, Currying is not compatible with vectorization.

Stuff I'm not sure about:
 * mag, normalize -- need to restrict domain to non-empty vectors
 * match, compose -- they are monoids, so there is theoretically no problem.
   They return functions: function call is not vectorized, which is a problem.
 * mod, rem
 * lerp, smoothstep, clamp (arg is a 3-tuple)
 * atan[y,x]
 * map, filter, reduce: curried, 2nd arg is list
 * merge record-list

Functions in std.curv that could be vectorized:
 * cmul, csqr
 * cross
 * identity
 * perp
 * cis

How can we make Curried functions compatible with vectorization?
 * I think that function call needs to be left-vectorized.
    * In Q'Nial, an Atlas is a list of functions. It can be applied directly
      to an argument, like a function, as is proposed here.
    * FP burns the [] characters for a combinator that maps a sequence of
      functions onto a function with atlas semantics.
    * This conflicts with array[i,j] notation, which needs to be replaced
      by array![i,j].
 * If 'map f' is vectorized over its first argument, then it can return a
   list of functions? Which can then be called using the left-vectorized
   function call.
 * In Curv, a binary operation has a single argument, which is a pair: f[x,y].
   In Haskell, a binary operation is curried: f x y. Does the Haskell design
   interfere with vectorization? Not in principle: Curv has plans to support
   sections, which is like Currying for binary ops. What semantics?
        (`+`2)2 == 4
        (`+`2)[1,2,3] == [3,4,5]
        (`+`[1,2,3])2 == [3,4,5]
        (`+`[1,2,3])[20,30,40] == [21,32,43]
    Is (`+`[1,2,3]) a function or atlas? If it is an atlas, then we need
    different atlas semantics than Q'Nial and FP: we need
        [f,g,h][a,b,c] == [f a, g b, h c]

If all functions were vectorized, and if vectorization was a composable
property, then new functions implemented in terms of existing vectorized
functions would themselves naturally be vectorized. However, that doesn't
happen most of the time. Can we design a set of composable vectorized operations
and programming idioms that are sufficient to express any new vectorized
operations we may need to write? A benefit of these operations and coding
guidelines is that it could result in code that can be automatically compiled
into performant, data parallel code for fast execution on CPUs and GPUs, which
in turn would lead to fast rendering, high frame rate animation and responsive
interactive graphics.

Indexing a Tensor
~~~~~~~~~~~~~~~~~
Tensors are indexed using a generalization of list indexing notation.
A tensor of rank *k* is indexed by a vector of count *k*.

For example, if ``M`` is a matrix, then ``M[i,j]`` retrieves the element
at row ``i`` and column ``j``, assuming ``i`` and ``j`` are integers.
This is just function call notation, where ``M`` plays the role of a function,
and the vector ``[i,j]`` plays the role of an argument.
``M[i,j]`` is equivalent to ``M[i][j]``, due to the representation of matrices
as nested lists.

Note: In theory, since ``42`` is a tensor of rank 0, it should be the case that
``42[] == 42``. However, this is not implemented.

Note: Tensor slicing is not implemented (yet).

Tensorized Numeric Operations
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
All of the built-in scalar numeric operations
are generalized to operate on tensors.

Unary operations (like unary ``-``) are extended to operate "elementwise"
on each element of a tensor.  For example::

  -[[1,2],[3,4]] == [[-1,-2],[-3,-4]]

Binary operations (like binary ``+``) are extended in two ways:

* Elementwise: If the arguments are two lists of the same count,
  eg ``[a,b]+[c,d]``,
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

``dot(A,B)``
  The tensor dot product ``A⋅B`` is a generalization of vector dot product
  and matrix multiplication.

  In the general case, A and B are tensors of at least rank 1.
  The final dimension of A equals the first dimension of B.
  Cut A into slices along its last axis,
  do the same with B along its first axis, 
  then combine each slice from A with each slice from B using *, 
  and finally perform a reduction using +. 
  The resulting tensor has rank equal to ``rank(A)+rank(B)-2``.

  If V is a vector and M is a matrix, then:
  
  * ``dot(V1, V2)`` is the dot product of two vectors.
    Same as ``sum(V1 * V2)``, or ``V1*V2`` in OpenSCAD.
  * ``dot(V, M)`` is the product of a vector and a matrix.
    It's like matrix multiply, treating V as a column vector,
    but the result is a vector.
    Same as ``sum(V * M)``, or ``V*M`` in OpenSCAD.
  * ``dot(M, V)`` is the product of a vector and a matrix.
    It's like matrix multiply, treating V as a row vector,
    but the result is a vector.
    Same as ``sum(transpose M * V)``, or ``M*V`` in OpenSCAD.
  * ``dot(M1, M2)`` is standard matrix multiplication (``M1*M2`` in OpenSCAD).

  This operation is equivalent to the ``Dot`` function in Mathematica,
  or to the following Curv definition::
  
    dot(a,b) =
      if (count a > 0 && is_list(a[0]))
        [for (row in a) dot(row,b)]  // matrix*...
      else
        sum(a*b)                     // vector*...
