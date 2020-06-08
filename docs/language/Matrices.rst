Matrices
--------
``idmatrix n``
  An ``n`` × ``n`` identity matrix.

``transpose a``
  The transpose of a matrix: a new matrix whose rows are the columns of the original.

``dot[a,b]``
  Matrix multiplication, a special case of the tensor dot product.
  If V is a vector and M is a matrix, then:
  
  * ``dot[V, M]`` is the product of a vector and a matrix.
    It's like matrix multiply, treating V as a column vector,
    but the result is a vector.
    Same as ``sum(V * M)``, or ``V*M`` in OpenSCAD.
  * ``dot[M, V]`` is the product of a vector and a matrix.
    It's like matrix multiply, treating V as a row vector, but the result is a vector.
    Same as ``sum(transpose M * V)``, or ``M*V`` in OpenSCAD.
  * ``dot[M1, M2]`` is standard matrix multiplication (``M1*M2`` in OpenSCAD).
