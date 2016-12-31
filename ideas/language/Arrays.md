# Arrays

## Array and Record Indexing should be overloaded?
In Javascript, array[i] and obj[key] are the same operator.
Ditto for Python lists and dictionaries.

In the original design cycle, this bugged me, and I wanted to be sure that
these are separate operators. But that leads to inventing weird operator syntax
for record indexing that nobody has seen before. Eg, like `rec.(key)`.

Here are the advantages of overloading:
* Don't have to invent weird operator syntax for record indexing.
* `update(structure, index, newval)` works for all compound data structures.
* A "path" into a compound data structure can be represented as a list of
  index values. Eg, compare to JSON-Path.
* Python and Javascript use the same design, so it's popular.

## Array Indexing Syntax
I've been unsure about the best syntax for indexing an array.

Currently I favour `a'i`.
The others are more worm-can-openey/yak-shavey.
I can always change this later.

In Curv, instead of writing `max([a,b])` and `matrix[i][j]`,
you instead write `max[a,b]` and `matrix'i'j`.
The chosen syntax is shorter, easier to read, easier to type.

0. Popular syntax `a[i]` conflicts with functional-style function call,
   and I've decided to prioritize the latter.
   * Maybe use `a[i]` and choose a different syntax for array literals,
     eg `{...}`.

1. `a(i)` is kind of cool, and second most popular syntax. `matrix(i)(j)`
   or `matrix(i,j)`, `bbox 0 0`. Used by Fortran, Ada, Matlab, Scala.
   It conflicts with module customization, which must use funcall notation.
   * Maybe a different syntax for accessing module elements.
     We don't need Module to be a subtype of List in Curv.
     * Elements are named by a special field name. Temp name: `__elements__`.
       Name is used when a module shape is serialized as JCSG.
       This also allows a module to reference its own elements by name.
     * `elements <module>` returns the elements.
   * Maybe function call is overloaded in modules.
     A record argument is customization, an integer argument is element
     selection, a string argument is field selection.
     * Does this interfere with `cube 10` for customizing the cube prototype?
       Must all shapes be customized using a record argument?
   * If funcall syntax is used for function call, array indexing and
     field selection, then a function can mimic any compound data type.
     That's interesting...
   * Scala uses a(i) on principle, not to mimic Fortran.
     Array is a subtype of Function. This adds a bit of expressive power,
     but it's not that big a deal.
   * From a beginner's mind perspective, a(i) is simpler because it
     requires less syntax: a(i) and f(x) are the same syntax.

2. Use `a'i` as an infix operator. Eg, `bbox'0'0`, `matrix'i'j`, `record'key`.
   Most readable, easiest to type of any infix symbol I've tested.
   Others: `@`, `!` (Haskell), `#`. `bbox@0@0` is not readable.
   No infix array index operator will be familiar or guessable syntax.

3. F# uses `a.[i]`. I thought of also using `record.{string}`.
   I implemented that, on the basis of similarity to the consensus syntax,
   but now I'm not too happy with this syntax either. Not sure why.
   * A lot of my current code is `p.[0]` and `p.[1]`.
     The verbosity bugs me.
   * Maybe the fact that `[i]` has a context sensitive meaning.
     `a.[[a,b,c]]` is a bit jarring.
   * Maybe the fact that it is so close to the consensus syntax, and
     yet different?

4. O'CAML uses `a.(i)`. Then, as a short form, `a.0` and `a.1`
   which are terse and readable, no worse than the `a.x` and `a.y` shortcuts
   in OpenSCAD. Then `a.[i,j]` is interpreted as `[a.(i),a.(j)]`.
   But then `a.i` has an entirely different meaning.

5. Use `a[i]` and choose a different syntax for array literals.
   * Maybe `{1,2,3}`.
     * translate{0,3} shape
     * crop{xmin=0} shape
     * What is `{}`? It kind of suggests the unification of arrays and records,
       similar to Objects in Javascript and Lua.
   * Maybe `(1,2,3)`.
     * What is `f(1,2,3)`?

```
    bbox = [
        [min[s1.bbox@0@0, s2.bbox@0@1],
         min[s1.bbox@0@1, s2.bbox@0@1]],
        [max[s1.bbox@1@0, s2.bbox@1@0],
         max[s1.bbox@1@1, s2.bbox@1@1]]
    ]
    bbox = [
        [min[s1.bbox!0!0, s2.bbox!0!1],
         min[s1.bbox!0!1, s2.bbox!0!1]],
        [max[s1.bbox!1!0, s2.bbox!1!0],
         max[s1.bbox!1!1, s2.bbox!1!1]]
    ]
    bbox = [
        [min[s1.bbox'0'0, s2.bbox'0'1],
         min[s1.bbox'0'1, s2.bbox'0'1]],
        [max[s1.bbox'1'0, s2.bbox'1'0],
         max[s1.bbox'1'1, s2.bbox'1'1]]
    ]
    bbox = [
        [min[s1.bbox@(0)@(0), s2.bbox@(0)@(1)],
         min[s1.bbox@(0)@(1), s2.bbox@(0)@(1)]],
        [max[s1.bbox@(1)@(0), s2.bbox@(1)@(0)],
         max[s1.bbox@(1)@(1), s2.bbox@(1)@(1)]]
    ]
    bbox = [
        [min[s1.bbox.[0].[0], s2.bbox.[0].[1]],
         min[s1.bbox.[0].[1], s2.bbox.[0].[1]]],
        [max[s1.bbox.[1].[0], s2.bbox.[1].[0]],
         max[s1.bbox.[1].[1], s2.bbox.[1].[1]]]
    ]
```

## Tensors
Curv supports arrays of numbers of arbitrary dimension,
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
Since we compile Curv directly to optimized machine code, this should
allow for fast vector operations.

## Broadcasting
In OpenSCAD, the arithmetic operations -A, A+B, A-B support pointwise
operation and "broadcasting" across arrays and arbitrary shaped trees,
following the convention of most array languages.

In Curv, *all* of the numeric operations support these semantics,
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

### Explicit Broadcasting
In Maple, broadcasting is specified explicitly using the `~` suffix:
```
-~x
x +~ y
sin~(x)
x ==~ y
```

Benefits:
* The non-broadcast version of the operators may run slightly faster due
  to the elimination of some conditional logic.
* Compile time type inference and optimization:
  `x*y` is known at compile time to return a number,
  therefore in `a*b+c*d`, the `+` can be specialized at compile time to
  a float add instruction with no type tests.
* This syntax distinguishes between normal and element-wise equality,
  which are semantically different.
  (Element-wise equality *must* be marked in some way, unless I go full APL/K
  and use extra marking for non-element-wise equality.)
* Works for all user-defined functions. (With the implicit approach, only
  user defined functions that are designed to broadcast will do so, and more
  complicated code may be required.)
* The extra syntax reminds you that this is a more expensive operation?

Drawback: it looks ugly. `M1 +~ M2` is not the usual math notation for
matrix addition.

Wait and see if this helps in any way with generating GLSL code.

Matlab uses a `.` suffix for this, in some contexts but not others.
However, `sin x` vs `sin~ x` works, doesn't quite work using `.`.
I also thought of `#`, which resembles a matrix,
but maybe the denser the symbol, the uglier it is.

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

 3. In Curv and OpenSCAD, matrixes are represented as nested lists.
    There is no 'matrix' type. Curv can't distinguish between a matrix
    and a list of 3D points (eg, the latter appears in the `polyhedron`
    arguments). Elementwise multiplication is something that makes sense
    in both contexts. Matrix multiplication is more specialized than this,
    so it needs to be a different operation.

Mathematica is a good language to for Curv to copy, because:
* It represents arrays as nested lists, just like OpenSCAD.
* It is consistent by design: Mathematica's symbolic algebra wouldn't work
  if the operators didn't have consistent mathematical properties.

Curv adopts 3 multiplication operators from Mathematica,
which in Curv are called `product` (*), `dot` (·)
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

In Curv, all of the numeric operators are generalized to operate on 'tensors'.
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
