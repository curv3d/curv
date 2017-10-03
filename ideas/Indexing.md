## Array Indexing

Change array indexing to be consistent with most array languages.

a[i] -- i is an integer or a list of indices
  eg, APL, numpy, R, Mathematica, Matlab

a[i..j] -- slice
a[..j]
a[i..]
a[..]

a[i,j,k] -- like a[i][j][k], except when slice notation is used.

index path list
  index(i,j,k) list == list[i,j,k]
  Number of elements in the path is not fixed at compile time.
  Suitable for pipelines, eg list >> index path

Vector swizzling: a[(X,Y)] or a'[X,Y].

A further modification:
* `x [...]` is a function call.
* Lists can be called like functions: the argument is a path list.
  a[0] is the first element in a list, m[i,j] indexes a matrix,
  and a(path) indexes a list using a path value.
  The [...] syntax only has one meaning, not two, so we eliminate
  the confusion around `a[[...]]` where the inner and outer brackets
  have different meanings.
* Unfortunately, a[1..3] is now a path,  not a slice.
  We change i..j so that it is an expression returning a list value,
  not a generator. `a[1..3]` is now a slice, and is no longer equivalent
  to `a[[1..3]]` (that equivalence is confusing).
* What about `a[..j]`, `a[i..]` and `a[..]`?
  In `i..j`, if `i` is omitted, it defaults to `0`.
  If `j` is omitted, then the result is a infinite/indefinite range value.
  Haskell supports this, so I could try to follow Haskell semantics to
  the limited extent that this is feasible.
   * `count(0..) == inf`
   * `dom(1..) == (0..)`
   * `(2..)[i] == i+2`
   * `"$(2..)" == "2.."`

Oops: `x[...]` as a function call conflicts with `{map: f}`.
* Use `thing'i` or `index path thing` to index a structure, list or string.
  Function call syntax is an abbreviation for indexing lists and strings,
  while for function call on a structure, S x means S.map x
  We still have list/string/structure polymorphism using index, dom and count.

-------
The ' syntax makes more sense. a'i is regular indexing (i is an integer),
while a'[i,j] is the vectorized form.

a[i] is regular indexing.
a[i,j] is a shortcut for vectorized indexing, equivalent to a[[i,j]].
I find the latter confusing (the inner and outer brackets have different
meanings, and the shortcut equivalence is not obvious).

Vectorized array indexing using list[index_list] seems to not be a familiar
or common operation in other languages. What we see instead is slice notation,
like in Python a[first:step:last], or multidimensional array indexing m[i,j],
which is very common in math-oriented languages.

Maybe I should consider a different design:
* Keep a'i as an alternate form of a[i]. Use a'[i,j] or a[[i,j]]
  or a[(i,j)] for vector swizzling.
  * Numpy allows a[[i,j,k]] for vectorized indexing, but the index is either
    an integer or a list of integers. Not a nested list.
    Ditto for Mathematica: a[[{i,j,k}]].
* Slice notation, a[start..end by step], with start, end and step all optional.
* Maybe use a'[i,j] exclusively for vector swizzling. Otherwise, a[i..j] and
  a[[i..j]] are sort-of-equivalent, which might be confusing.
* Multidimensional array indexing: a[i,j,k], possibly extended with slices.
* In NumPy, you can use a[(i,j)] instead of a[i,j] for multidimensional indexing
  I.e., indexing a nested data structure using a "path" (a list of indexes).
* `a[...] := x` is going to be legal. Which structures `...` make sense
  in this context?
  * Scalar index.
  * Contiguous slice of a list. a[i..j] := list, the slice index and list
    can have different counts.
  * Path, a list of scalar indices.
  * Numpy allows `a[[1,2,3]] = [10,20,30]`.
