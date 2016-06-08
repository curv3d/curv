# Arrays

## Tensors
TeaCAD supports arrays of numbers of arbitrary dimension,
plus the usual linear algebra operations on vectors and matrices.
A vector is a list of numbers, a matrix is a list of vectors, and so on.

We can describe generalized arrays as *tensors*, where a number like 42
is a tensor of rank 0, a vector like [1,2,3] is a tensor of rank 1,
a matrix has rank 2, and so on.
A tensor of rank N+1 is represented by a list of rank N tensors which
all have the same dimensions.

`dim(A)` returns the dimensions of a tensor, as a vector.
As a special case, if A is a number, the result is `[]`.
`len dim A` is the rank of the tensor A.
(I don't have a good use case for this operation, yet.)

Indexing a tensor.
* A@(i,j) is the same as A@i@j if i and j are integers.
* A@(a..b,x..y) is a rectangular slice.
* In general, for each axis, the index can be a number, a list, or a range,
  and the range syntaxes are `..`, `i..`, `..j` and `i..j`.
* For a tensor of rank N, you can specify less than N indexes

Performance note: The Curv "boxed value" format represents numbers in memory
as 64 bit IEEE floats, and it represents lists of numbers as contiguous arrays
of 64 bit IEEE floats. This means that vector instructions (eg, SSE or AVX
on Intel) can be applied directly to boxed vectors, without copying the data.
Since we compile TeaCAD directly to optimized machine code, this should
allow for fast vector operations.

## Broadcasting
In OpenSCAD, the arithmetic operations -A, A+B, A-B support pointwise
operation and "broadcasting" across arrays and arbitrary shaped trees,
following the convention of most array languages.

In TeaCAD, *all* of the numeric operations support these semantics,
including A\*B, abs(A), max[A,B], sin(A), and so on.

What does this look like for `max`?
* `max[5, [2,4,6]] == [5,5,6]`
* `max[[1,2], [3,0]] == [3,2]`.
  If we interpret the argument to max as a matrix, instead of as a list of
  two vectors, then we've computed the maximum of each column.
* To compute the maximum of each row in matrix M,
  you can write `max transpose M`.
* To compute the maximum value within the matrix, you can write `max max M`
  or `max concat M`.
* Or `max flatten T` to find the maximum value in an arbitrary tensor T
  of rank >= 1.

Here's sample code for broadcasting unary operation `f` across a list:
```
  broadcast(f)(xs) =
    if (is_list xs)
      map(broadcast f)xs
    else
      f(xs);
```

I plan to base the design on Mathematica (Wolfram language),
because it is a well known, fully worked out and consistent design,
based on a dynamically typed language where multi-dimensional arrays
are represented as nested lists.

<!--
Note: There may be a performance implication to generalizing ordinary
numeric operations to operate on tensors. It's convenient, but in the
absence of type information, the compiler can't generate efficient
code for an expression like `x+y` (ie, a floating point add instruction),
because x and y might be non-scalar.

So *maybe* I want to syntactically distinguish tensor operations from
scalar operations. Of course, that will be ugly. One idea is to use
`#` to indicate a tensor operation, like this:
```
-#x
x +# y
sin#(x)
```
So the ugly `#` is a visual reminder that you are doing something expensive.
Alternatively, we leave it out, and if you want higher performance,
then you add type annotations.

One context where `#` is semantically useful is equality:
`x==y` compares two tensors for equality, returning `true` or `false,
while `x==#y` performs an element-by-element comparison, returning an array
of booleans. Both are useful in computational geometry: eg, GLSL supports both
forms of equality on vectors.
-->

## Generalized Multiplication
Here are 3 ways to generalize scalar multiplication to vectors and matrices:
* Elementwise multiplication (called Hadamard product for matrices).
* Dot product and matrix multiplication.
* Vector cross product.

In OpenSCAD, as in Matlab, the syntax `x*y` is used for both scalar
and matrix multiplication.
This is convenient, if you are focussed on matrix multiplication,
but it creates semantic inconsistencies, and other scientific/matrix-oriented
programming languages make different choices.
Mathematica, R, and Euler Math Toolbox use `x*y` for scalar and
elementwise array multiplication, and a different syntax for matrix
multiplication. I'll argue that this latter design is better.

 1. Each operator should be designed to have consistent mathematical
    properties across all operands. For example, elementwise multiplication
    is associative, commutative and distributive for all operands,
    and is a monoid with identity element 1.
    * This assists humans in reasoning about code without knowing the types
      of operands. When you look at an arithmetic expression, you by default
      think about what it means for scalar operands. You assume,
      perhaps unconsciously, that x*y == y*x, and that's its okay to
      rearrange the code under this assumption. (But matrix multiply is
      not commutative.)
    * It also assists an optimizing compiler in optimizing expressions without
      knowing the types of operands, which is important for future optimization
      work.

 2. If we are going to generalize existing scalar operations to work on arrays,
    then we should be consistent. Elementwise operation (broadcasting) is
    a consistent generalization.

 3. In TeaCAD and OpenSCAD, matrixes are represented as nested lists.
    There is no 'matrix' type. TeaCAD can't distinguish between a matrix
    and a list of 3D points (eg, the latter appears in the `polyhedron`
    arguments). Elementwise multiplication is something that makes sense
    in both contexts. Matrix multiplication is more specialized than this,
    so it needs to be a different operation.

Mathematica is a good language to for TeaCAD to copy, because:
* It represents arrays as nested lists, just like OpenSCAD.
* It is consistent by design: Mathematica's symbolic algebra wouldn't work
  if the operators didn't have consistent mathematical properties.

TeaCAD adopts 3 multiplication operators from Mathematica,
which in TeaCAD are called `product` (*), `dot` (·)
and `cross` (⨯). (The Mathematica names are Times, Dot and Cross.)

`product[x,y]` or `x*y` performs pointwise multiplication.
So 2*3 == 6, 2*[1,2,3]==[2,4,6], [1,2,3]*[1,2,3]==[1,4,9],
and so on for higher dimensions. It computes the pointwise (Hadamard)
product of two matrices of the same dimensions.
`product` is commutative, associative and distributive for all operands,
and is a monoid with identity element 1.

Generalized vector and matrix product: `A +* B`:
* `V1 +* V2` is the dot product of two vectors, like `V1*V2` in OpenSCAD.
  Same as `sum(V1 * V2)`.
* `V +* M` is the product of a vector and a matrix, like `V*M` in OpenSCAD.
  It's like matrix multiply, treating V as a column vector,
  but the result is a rank 1 vector.
  Same as `sum(V * M)`.
* `M +* V` is the product of a vector and a matrix, like `M*V` in OpenSCAD.
  It's like matrix multiply, treating V as a row vector, returning a vector.
  Same as `sum(transpose M * V)`.
* `M1 +* M2` is standard matrix multiplication, like `M1*M2` in OpenSCAD.
  In K, `+/*` is vector dot product while `(+/*)\` (plus OVER times) EACHLEFT
  is matrix product.

A +* B
[[a,b],[c,d],[e,f]] +* [[u,v,w],[x,y,z]]
sum[ [[au,av,aw],[cu,cv,cw],[eu,ev,ew]], [[bx,by,bz],[dx,dy,dz],[fx,fy,fz]] ]

TA = transpose A == [[a,c,e],[b,d,f]]

sum[
  for (i=[0:len(TA)-1])
    let (tarow=TA@i,
         brow=B@i)
    for (j=tarow)
      for (k=brow)
        j*k


These are just special cases of a more general operation that is defined
for tensors of arbitrary rank >= 1.
`A +* B` is called `A.B` in Mathematica and `A+.×B` in APL.
The last dimension of A must match the first dimension of B.
If A has dimensions [i1,i2,...,i(m-1),i(m)],
and B has dimensions [j1,j2,...,j(n-1),j(n)],
(where i(m) == j1)
then the result has dimensions 
[i1,i2,...,i(m-1),j2,...,j(n-1),j(n)].
Thus applying `+*` to a rank M and a rank N tensor gives a rank M+N-2 result.

`+*` is associative, but not commutative, and has no universal identity element.

`cross(V1,V2)` is vector cross product.
(not commutative, not associative, no identity)

## Boolean Arrays
This is a topic for future consideration.

A majority of scientific and array languages generalize the relational and
boolean operators to operate pointwise across arrays, yielding arrays
of booleans as results.  Even GLSL has pointwise relational operators
and vectors of booleans, so there must be a use in geometric programming.
In GLSL, you can feed a bool vector into any() and all().

In TeaCAD, all of the numeric operators are generalized to operate on 'tensors'.
But the relational and boolean operators shouldn't be overloaded this way.
Instead, we'll use different names for the pointwise versions. Why:
* `==` can't be generalized to pointwise operation across arrays since it
  is already defined to test two lists for equality.
* The short-circuit semantics of `and` and `or` don't make sense with arrays.
* `if` does not accept a boolean array as a condition. It would create a pitfall
  if `if (x > y && b)` could fail at runtime due to `x > y` returning an array,
  or `a && b` returning an array.

Here are proposed names for pointwise relational and boolean operators:
```
    a ==. b  a !=. b
    a >. b   a <. b
    a >=. b  a <=. b
    a &&. b  a ||. b
    !. a
```
Matlab uses `.==` but the postfix `.` is more readable.

Alternatively, instead of adding a family of rarely used infix operators,
these operations can be added at the library level as ordinary functions.

How do you test if all elements of a list (array) satisfy some relation
or predicate? Test if at least one element does?
```
    all(V ==. e)
    all(flatten A ==. e)
    find(x->x!=e)V == null
    all(map(x->x==e) V)
    all[for (x=V) x==e]
```

In APL, and many of its successors, the boolean values are 0 and 1,
and are numbers, so boolean arrays are numeric arrays, and that provides
much of the extra expressiveness that justifies boolean arrays.
If we wanted to go down this path, we could define false=0 and true=1.

Now there's a choice:
* In some languages, type Bool is a subtype of the integers containing
  only the values 0 and 1. If boolean operators are restricted to arguments
  of type Bool, then you get the familiar algebraic properties of boolean
  operators: `and` and `or` are commutative. (Strongly typed languages,
  but also APL.)
* In Python, `True` and `False` are boolean values that print as themselves,
  but in most contexts they are equivalent to `1` and `0`. Eg, `True==1`.
* In other languages, the boolean operations are generalized to work on all
  integers, or on all numbers.
  * Eg, 0 is false, all other numbers are true. All numbers have a boolean
    interpretation. So `X and Y` returns zero if either argument is 0,
    otherwise it returns `X` (so it is non-commutative).
    C, Python, and others.
  * In K, the boolean values are restricted to 0 and 1, and the boolean
    operations are numeric operations with non-boolean behaviour when given
    non-boolean arguments. `X & Y` is the `min` operation, which is commutative.

What makes sense to me is to have unique truth values. Only one value is true,
only one value is false. The logical operations are restricted to truth values,
and obey all of the standard algebraic properties, like commutativity.
Either:
* true and false are distinct values, different from numbers, or
* the truth values are 0 and 1, and we supply true and false as aliases.

## Bibliography
The APL family of array languages are of interest.
* K is notable, due to its simplicity, and also because multi-dimensional arrays
  are represented as lists of lists, as in OpenSCAD.
  http://web.archive.org/web/20050504070651/http://www.kx.com/technical/documents/kreflite.pdf
* Mathematica is also a dynamically array typed language that uses lists of
  lists for multi-dimensional arrays. Eg, the Dot function is +.* from APL.
