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

``is_list value``
  True if the value is a list, false otherwise.

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
