Lists
-----
A list is a finite, ordered sequence of values.
For example, ``[1,2,3]`` is a list of 3 numbers.

List Constructors
  A list constructor is a comma-separated list of expressions or value
  generators, inside square brackets or parentheses.

      The parenthesis notation is deprecated. If used, either the list is
      empty ``()`` or contains at least one comma. The final expression or
      value generator may be followed by an optional comma.
  
  For example::
  
    [] or ()
    [1] or [1,] or (1,)
    [1,2] or (1,2)

  A value generator is a kind of `statement`_
  that adds zero or more elements to the list being constructed.
  The simplest value generator is just an expression,
  which adds one element to the list.
  
  The spread operator (``...list_expression``) interpolates all of the elements
  from some list into the list being constructed.
  For example, this concatenates two lists::
  
    [...a, ...b]
  
  Complex value generators may be composed from simpler ones using blocks and control structures,
  as described in `Statements`_.
  This syntax is a generalization of the *list comprehensions* found in other languages.
  For example, this yields ``[1,4,9,16,25]``::
  
    [for (i in 1..5) i^2]

.. _`statement`: Statements.rst
.. _`Statements`: Statements.rst

Range constructors
  ``i .. j``
    Returns the ascending list of numbers: ``i``, ``i+1``, ``i+2``, ... up to ``j`` inclusive.
    For example, ``1..10`` is ``[1,2,3,4,5,6,7,8,9,10]``.
    Same as ``i .. j by 1``.

  ``i .. j by k``
    Same as ``i..j``, except that the step value is ``k`` instead of ``1``.
    The step value may be positive or negative, and need not be an integer.
    For example, ``1..0 by -0.25`` is ``[1, 0.75, 0.5, 0.25, 0]``.
    
    We assume that ``j-i`` is intended to be an integer multiple of ``k``.
    It might not be an exact multiple, due to floating point inaccuracy,
    so the number of list elements is computed using rounding, like this::
    
      let n = round((last - first)/step) in
      if (n < 0) 0 else n + 1
    
    As a result of using this algorithm, the number of list elements will be
    correct (under the stated assumption), but the final list element might
    be larger or smaller than expected, due to floating point inaccuracy.
    Here's an example: ``0.1 .. 0.3 by 0.2`` returns ``[0.1, 0.30000000000000004]``.

  ``i ..< j``
    Same as ``i..j`` except that the final element in the sequence is omitted.
  
  ``i ..< j by k``
    Same as ``i..j by k`` except that the final element in the sequence is omitted.

List Indexing
  ``a[i]``
    The i'th element of list ``a``, if ``i`` is an integer.
    Zero based indexing: ``a[0]`` is the first element.

List Slicing
  ``a[indices]``
    Returns ``[a[indices[0]], a[indices[1]], ...]``,
    where ``indices`` is a list of integers.
    For example, ``a[0..<3]`` returns a list of the first 3 elements of ``a``.

Array Indexing
  ``a[i,j,k,...]``
   Index into a multidimensional array, which is represented by nested lists.
   If the indices ``i``, ``j``, ``k``, ... are all integers,
   then this is equivalent to ``a[i][j][k]...``.
   See `Tensors`_ for more information about multidimensional arrays.

.. _`Tensors`: Tensors.rst
    
*Syntax note:* The Curv juxtaposition operator (``a b``) is overloaded.
If ``a`` is a function, then this is a function call with argument ``b``.
If ``a`` is a list, then this is list or array indexing,
and ``b`` is a list of indices.

``count a``
  The number of elements in list ``a``.

``is_list value``
  True if the value is a list, false otherwise.

``concat aa``
  This is the list concatenation operator.
  ``aa`` is a list of lists. The component lists are catenated.
  For example, ``concat[[1,2],[3,4]]`` is ``[1,2,3,4]``.
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

``reduce [zero, f] a``
  Using binary function ``f``,
  iteratively combine all of the elements of list ``a`` into a single value,
  recursing on the left.
  For a 4 element list ``[a,b,c,d]``, this will compute::

    f[f[f[a,b],c],d]

  If the list has zero length, the result is ``zero``.

``contains [list, x]``
  A predicate, returns ``#true`` if ``x`` is an element of ``list``.
