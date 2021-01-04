Arrays
======
Curv supports generalized multi-dimensional rectangular arrays,
representing them as nested `lists`_.
For example, here is the Curv representation of a 3x3 identity matrix,
which is a 2-dimensional array::

    [[1,0,0],
     [0,1,0],
     [0,0,1]]

Multi-dimensional arrays are used for doing linear algebra,
which is the basis for many algorithms in computer graphics.

.. _`lists`: Lists.rst

The Rank of an Array
--------------------
A *k*-dimensional array of numbers is said to have rank *k*.
The **rank** of an array is the number of integer indices that are required
to retrieve an element. If ``a`` is an array of rank 3, then ``a.[i,j,k]``
is an element of ``a``.

The **dimensions** of an array of rank *k* can informally be thought of
as a *k*-element vector containing the number of elements along each axis.

Some array ranks have special names, which come from linear algebra:

* A number, by itself, is called a **scalar**, which is an array of rank 0,
  and has dimensions ``[]``.
* A **vector** with *n* elements is an array of rank 1,
  and has dimensions ``[n]``.
  A vector is represented by a list of numbers.
* A **matrix** with *m* rows and *n* columns is an array of rank 2,
  and has dimensions ``[m,n]``.
  It is represented by a list of *m* vectors of count *n*.
* In general, an array of rank ``n+1`` is represented by a list
  of arrays of rank ``n``, all with the same dimensions.

Indexing an Array
-----------------
Arrays are indexed using a generalization of list indexing notation.
An array of rank *k* is indexed by *k* indices.

In the array indexing syntax ``A.[i]``, the index phrase *i* is a `generator`_
that produces a sequence of up to *k* index values. Usually, *i* is a
comma-separated list of index expressions.

.. _`generator`: Generators.rst

For example, if ``M`` is a matrix, and ``i``, ``j`` are row, column indices,
then ``M.[i,j]`` retrieves the element at row ``i`` and column ``j``.
``M.[i,j]`` is then equivalent to ``M.[i].[j]`` due to the representation
of matrices as nested lists.

For example, if ``ix`` is a list of indices,
then you can use it to index an array using the syntax ``A.[...ix]``.

As a special case, since ``42`` is a scalar (an array of rank 0),
you can index it with 0 indices, and ``42.[]`` returns ``42``.
Although you wouldn't type ``a.[]`` directly, this is an edge case that
needs to be handled correctly when you are generating lists of indices
in generic algorithms that work on arrays of varying rank.

Slicing an Array
----------------
Array slicing is a generalized kind of indexing, where instead of
retrieving a single scalar element, you retrieve a subset of the array
elements, packaged as another array. This is like list slicing, generalized
to multiple dimensions.

To construct an array slice, you index an array of rank *k* with *k*
indices, but this time, each index is either a scalar, or it is a vector
of indices for that particular axis.

Let's begin with vector slicing, which is the simplest case.
Suppose you have a vector ``v`` in 3-D space, containing the elements
``[x,y,z]``, and you want to project this onto the X-Y plane by selecting
the first two elements. You can write this as a slice::

    v.[[X,Y]]

Here, there is a single index ``[X,Y]``, which is a vector. The predefined
constants ``X`` and ``Y`` are ``0`` and ``1``.
This code is equivalent to ``[v.[X], v.[Y]]``. However, the slice notation
may be faster when Curv is compiled to CPU or GPU machine code,
because the slice notation is compiled into a `vector swizzling`_ instruction,
which is faster than extracting the vector elements one at a time and then
reassembling them. It's perfectly legal for index values to be non-consecutive
or appear multiple times (like ``v.[[X,X,X,Z]]``)
and you will sometimes see this in GPU graphics code.

.. _`vector swizzling`: https://en.wikipedia.org/wiki/Swizzling_(computer_graphics)

Next, let's consider the multidimensional case.
If all of the indices are vectors, then the result is another array
of equal rank. Suppose we start with a matrix *M*,
and we index it with two vectors *v1* and *v2* of count *n1* and *n2*::

    M.[v1,v2]

Then the result will be another matrix of dimensions ``[n1,n2]``.

For example, consider this matrix::

     M = [[1,0,0,0],
          [0,2,0,0],
          [0,0,3,0],
          [0,0,0,4]]

We can extract a 2x3 slice from the upper left corner
by writing ``M.[0..1, 0..2]``::

    [[1,0,0],
     [0,2,0]]

If some of the indices in a multi-dimensional array slice are scalars,
then the resulting array will have a smaller rank than the original array.
We can extract the 3rd column from M by writing::

    M.[0..3, 2]

which gives ``[0,0,3,0]``.

Generalized Scalar Operations
-----------------------------
All of the built-in scalar numeric and boolean operations
are generalized to operate on arrays.

Unary operations (like unary ``-``) are extended to operate "elementwise"
on each element of an array.  For example::

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
For example, ``max[a,b,c,d]`` is treated as ``max[max[max[a,b],c],d]``.

These generalized operations obey almost all of the same algebraic laws
as their scalar versions. For example, generalized multiplication is
commutative, associative, and has an identity element of ``1``::

  [a,b]*[c,d] == [a*c,b*d] == [c*a,d*b] == [c,d]*[a,b]   // commutative
  1*[a,b] == [1*a,1*b] == [a,b]                          // identity element

Note: For matrix arguments, ``a*b`` performs elementwise (Hadamard)
multiplication, and not standard matrix multiplication. This is because it
would be bad design to overload the same symbol with two operations that obey
different algebraic laws. For standard matrix multiplication, use ``dot``,
the array dot product.

Other Array Operations
----------------------

``dot[A,B]``
  The array dot product ``A⋅B`` is a generalization of vector dot product
  and matrix multiplication.

  In the general case, A and B are arrays of at least rank 1.
  The final dimension of A equals the first dimension of B.
  Cut A into slices along its last axis,
  do the same with B along its first axis,
  then combine each slice from A with each slice from B using ``*``,
  and finally perform a reduction using +.
  The resulting array has rank equal to ``rank(A)+rank(B)-2``.

  If V is a vector and M is a matrix, then:

  * ``dot[V1, V2]`` is the dot product of two vectors.
    Same as ``sum(V1 * V2)``, or ``V1*V2`` in OpenSCAD.
  * ``dot[V, M]`` is the product of a vector and a matrix.
    It's like matrix multiply, treating V as a column vector,
    but the result is a vector.
    Same as ``sum(V * M)``, or ``V*M`` in OpenSCAD.
  * ``dot[M, V]`` is the product of a vector and a matrix.
    It's like matrix multiply, treating V as a row vector,
    but the result is a vector.
    Same as ``sum(transpose M * V)``, or ``M*V`` in OpenSCAD.
  * ``dot[M1, M2]`` is standard matrix multiplication (``M1*M2`` in OpenSCAD).

  This operation is equivalent to the ``Dot`` function in Mathematica,
  or to the following Curv definition::

    dot[a,b] =
      if (count a > 0 && is_list(a.[0]))
        [for (row in a) dot[row,b]]  // matrix*...
      else
        sum(a*b)                     // vector*...
