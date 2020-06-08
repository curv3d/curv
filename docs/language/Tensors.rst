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
For example, ``max[a,b,c,d]`` is treated as ``max[max[max[a,b],c],d]``.

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

``dot[A,B]``
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
      if (count a > 0 && is_list(a[0]))
        [for (row in a) dot[row,b]]  // matrix*...
      else
        sum(a*b)                     // vector*...
