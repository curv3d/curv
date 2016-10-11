# Loops
Expressive and efficient iteration, without using recursion.

## Fortran
I've designed an embedded imperative sublanguage for OpenSCAD.
It supports imperative programming idioms, including efficient
array update and the quicksort algorithm, without breaking
the referential transparency of function calls.
It's an ugly feature, though, due to the introduction of a large
embedded sublanguage.

## Common Lisp
Richard Waters' "Series" package. Somebody proposed it for OpenSCAD.
It's interesting, but overly complex. It would have to be completely redesigned
to fit into Curv.

## Scheme
I'm considering the named-let feature. Rename it as `loop`,
as in
```
loop id(x1=init1, x2=init2, ...) ...body that calls id(...) recursively...
```

## Sisal
Sisal is "pure functional programming for Fortran programmers".
It provides an interesting syntax for writing iterative array code,
that compiles to efficient code and doesn't depend on recursion.
Maybe some ideas could be borrowed for Curv?

It has two loop constructs.
* Parallel `for` is roughly equivalent to the OpenSCAD `for` loop.
  Iterations can be performed in any order, or in parallel.
* Sequential `for` is roughly equivalent to the OpenSCAD "C-style for".
  It has initialization, test and update sections.

These loop constructs are more complex, but also more flexible: complex loops
with multiple iteration variables are easier, less cumbersome to express than
in OpenSCAD.

On the downside, I don't see any way to implement the quicksort algorithm
(the original, efficient, destructive update version). The Sisal distribution
has a Haskell-style qsort implementation, labelled quicksort.

### `for` loop (parallel)

The 'for' loop is restricted in power so that the iterations can be performed
in any order, or in parallel. So, you can't capture information in one iteration
so that it affects the output of the next iteration.

The OpenSCAD 'for' loop has the same limitation, and I'd say that both
constructs have the same expressive power.

for {range}
  {optional body}
returns {returns clause}
end for

simple {range} is
  i in 1, n         // denotes a range of integers
  a in counters     // counters is an array, a is an element
  x in myarray at i // myarray[i]==x
compound {range} is
  R1 cross R2   // nested loop, basically
  R1 dot R2     // loop over both at same time, terminate when shorter range
                   is exhausted.

{loop body}
Contains definitions of variables that get a new value on each iteration.
Each variable denotes an array within the returns clause.
A variable X can be assigned conditionally, which means you are filtering
the source range.
(Not required, since a definiens expression can be used directly in
the returns clause, without being named.)

{returns clause}
A for expression can return multiple values.
Each return expression is an aggregation clause (constructs an array)
or a reduction clause (typically reduces an array to a single value).

Reduction operators include
  sum <array expression>
  product <array expression>
  ...

### sequential loops: for-initial-while
```
for initial
   i := array_liml(A);
   tmp := A[i];
   running_sum := tmp
while i < array_limh(A) repeat
   i := old i + 1;
   tmp := A[i];
   running_sum := old running_sum + tmp
returns value of running_sum
        array of running_sum
end for
```
I'd say that this construct seems similar in expressive power to
the OpenSCAD proposed 'C-style for' loop.
