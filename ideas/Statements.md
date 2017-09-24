# New Syntax 3 (Sept 2017)

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

## generalized scripts
Right now, `tests/curv.curv` is a sequence of assert statements (each
terminated by `;`), followed by `{}`, which is an arbitrary return value.
Right now, all Curv scripts are expressions that yield a value.

The `{}` trick is ugly. Maybe a script should be interpreted as either
an expression or a statement, depending on its syntax. This means `file`
is no longer a function, but a metafunction. A call to `file` can yield
an arbitrary Meaning type.

What are the implications? How useful is this? There's not much point in
a script exporting a definition or binder or generator:
export a module/record/list instead.

In an earlier revision of Curv, `a;b;c;` was a block that yields an empty list.
That design would also address my issue.

## `while` loops in list/record comprehensions

Right now, `for` loops are legal in list and record comprehensions,
but not `while` loops.

To fix this, I would need to at least support `a := b` redefinition phrases
(assignment actions) in list/record comprehensions.

To assign a variable, it must first be defined using `var a := b`.
And the definition must be part of the same linear action sequence:
it can't be embedded in a subexpression, because sequential evaluation
isn't defined in the expression world. List and record constructors are
subexpressions.

So, it seems that I will also need to support `var a := b` definitions
in list/record comprehensions. This creates ambiguity in a record constructor:
is a sequential variable definition local (to support while loops) or does
it define a field?

I could make record and module syntax more distinct. Eg, use `;` in modules
and `,` in records.

## Semicolons
I hate that definitions end with ',' in std.curv but end in ';' in a top level
block. Copying definitions from one context to another requires syntax change.

So allow ';' in both contexts. { def1; def2; ... }

Right now, ';' creates a block everywhere, even in braces.
{x=1;echo x} is legal, yields {} plus an echo effect.
(comma binds looser than semicolon--the opposite of English)

With this proposed syntax change, I think that ',' and ';' no longer have
a precedence relation.
  program ::= list EOF
  list ::= commas | semicolons
  item ::= ...
Mixing commas and semicolons at the same level will be a syntax error.

The meaning of semicolon is now context dependent. Forms a block at top level
or in parentheses. In braces, you get a module. In brackets, probably illegal.

## Mixed Modules/Records
For defining shapes, I have considered code where the define_shape argument
contains some definitions, then a `...` clause that inherits fields from another
structure. Eg,
    cube = make_shape {
        inradius r = make_shape { <cube constructor> };
        circumradius r = make_shape { <cube constructor> };
        ... inradius 1
    }

The recursive `use` operator is hard to implement.
So, I thought, introduce `...` as a field generator, allow field generators
and definitions to be combined in a structure literal. That's easier because
the bindings brought in by `...` don't have to be known during analysis.

If I allow definitions and field generators to be mixed in the same structure
literal, then what happens if there is a name conflict?
* run time error
* override. The field generator overrides the definition.
  Field generators are supposed to override each other, left to right, so that
  defaults and overrides can be programmed, like in javascript.
  If an fgen overrides a definition then the fgens should be written after the
  definitions, for clarity. Maybe that's enforced.

## Mixed Recursive/Sequential Definitions
If we can mix definitions and field generators,
can we mix recursive and sequential definitions?
Yes, as previously designed.

## Statements
There are a number of contexts where you can include actions as statements:
* a block
* a list constructor
* a structure constructor

The latter 2 contexts allow actions as statements, but not local bindings.

Currently, a list constructor can use `for` as a generator, but not `while`.
To fix this, permit local bindings in a list constructor.
What's the syntax?
* Currently, (a,b,c) is a list, (a;b;c) is a block.
  Now definitions are permitted in both contexts.
