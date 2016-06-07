# Functions
Like OpenSCAD2.

Supports pattern matching on lists in formal parameters.

Function literal syntax:
    atom -> expr
where
    atom ::= identifier | parenthesized-formals | bracketed-formals
    parenthesized-formals ::= '(' formals ')'
    bracketed-formals ::= '[' formals ']'
    formals ::= empty | formal-list
    formal-list ::= formal | formal-list ',' formal
    formal ::= identifier | bracketed-formals

I like the look of `map(x->x+1)`. Compare:
* `map(x->x+1)`
* `map(\x x+1)`
* `map(function(x) x+1)`

Function call syntax as in OpenSCAD2, with extensions:
    selection ::= selection list-literal
    selection ::= selection object-literal

`compose(`*list-of-functions*`)`
> a monoid, whose identity is the identity function.
> Each function must be callable with a single argument (except the last).
> Eg, compose a sequence of geometric transformations:
> `compose[translate(t),rotate(r),translate(-t)]`.

## Autocurrying
Let's say that `union` is defined like this:
```
  union(children=null, r=0) =
    if (children==null)
      c->union(children=c, r=r)
    else
      ...;
```
Then you have some flexibility in how you invoke `union`:
```
  union(children, r=3) // what's obvious from the prototype
  union children       // what's obvious from the prototype
  union() children     // auto-curried
  union(r=3) children  // auto-curried
```

## Unary Functions
A unary function is one that can be legally called with a single argument.

`id` is the identity function: a special unary function
that returns its argument. We have a requirement (from Transformations)
that `id==id` is true and `id==T` is false for a transformation T.
It should also print as `id`.

`compose[f1,f2,...]` composes a list of zero or more unary functions.
This is a monoid with identity `id`.

<!--
`mapf(f)(fseq)` -- fseq is a list of unary functions. Compose f with each
element of fseq, return a list of unary functions. Too abstract, no use cases.
`mapf(f)(fseq) = map(g->compose[f,g])fseq;`

`foldm(m)(fseq)` -- m is a monoid, fseq is a list of unary functions.
Returns a unary function. Too abstract. Eg,
* foldm(all) == allp
* foldm(sum) maps a list of unary numeric functions onto a unary function f(x)
  that returns the sum of applying each function in the list to x.
  So foldm(sum)==x->sum [for (f=fseq) f x]
-->

In mainstream functional languages, all functions are unary,
and the syntax `(a,b,c)` is a tuple value.
That makes the function call operator more orthogonal and expressive,
but adds a new kind of value that competes with lists and objects,
which adds complexity. This particular change doesn't seem to help with
the goals of TeaCAD, and may result in big changes to the language design.

## Predicates
A predicate is a unary function that returns a boolean.
Predicates define a subset of the set of all values.

Predicates are used in the argument to filter, in parameter and
argument assertions.

A type predicate defines a set of values that support
some set of operations. In OpenSCAD, idioms like `abs(X)==undef`
are used to test if X is a number, but that won't work in TeaCAD
due to strict argument checking. So we provide type predicates
instead, and you use `Num(X)` to test if X is a number.

Here are some useful type predicates:
* Bool
* Num
* Int
* String
* List(type=Any,len=null)
* Func -- all values that can be called using function call notation.
* Obj -- all values with name/value attributes accessed using `.` notation.
* Seq -- all sequence values that support len(X), X@i, etc.

By Occam's Razor, predicates are better than 'type values' for specifying types.

Some special predicates:
* Any(p) = true
* None(p) = false

These predicates might be useful:
`==X`, `!=X`, `<X`, `>X`, `<=X`, `>=X`.

Here are some boolean operations on predicates:
* allp[P1,P2,...] -- a monoid that returns the conjunction of a set
  of predicates, which is a predicate. Identity element is `any`.
* somep[P1,P2,...] -- a monoid that returns the disjunction of a set
  of predicates, which is a predicate. Identity element is `none`.
* notp(P)(x) = !P(x)

Maybe these boolean operations should also have an operator form?
`p&q`, `p|q`, `~p`.

Eg,
* filter(Int & >= 0)
* >=0 & <1
* Tensor = Num | List(Tensor)
  * recursion is okay here, because `f|g` compiles to `x->f(x)||g(x)`

The Haskell prelude has functions for `all o map(P)` and `some o map(P)`.
In Haskell these are `all(P)list` and `any(P)list`.
In Scheme SRFI-1, these are `every(P)list` and `any(P)list`.
Maybe call them all_map and some_map?
