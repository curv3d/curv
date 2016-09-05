# Functions
Like OpenSCAD2.

**Keyword parameters.**
Original plan is to support this, as in OpenSCAD and OpenSCAD2.
It's complicated to implement, so it's been delayed in the Curv prototype.
In Javascript, object literals are used instead, and Curv has records, so
there's little need for keyword parameters, outside of OpenSCAD compatibility.
Keyword parameters start to look redundant.
But then, pattern matching on records are needed as a full replacement. Eg,
```
cube(size, {center:is_bool=false}) = ...;
cube(10);
cube(20, {center=true});
```

OpenSCAD2 object parameterization (now module parameterization) relies on
optional keyword parameters. This could be replaced by a single record
parameter, but, it's one or the other.
* Records are used to represent sets of model parameters.
  So using M(record) for model customization, that looks like a good design.
  Eg, `lollipop{height=10, radius=5}`.
* Check out my cylinder implementation using `switch`.
  It's based on records and pattern matching.
  The keyword parameter version would need 7 parameters
  and complex argument parsing logic.

**Optional parameters.**
This is more useful than keyword parameters, and I'm sure it will get
implemented sooner. However, in the absence of optional parameters,
one could consider implementing ImplicitCAD style curried arguments.
That is, f(x,y) === f(x)(y).

**Pattern matching in formal parameters.**
* Pattern matching on lists is something I definitely want.
  Eg, `f[x,y]=...`. Add default values and you support a list argument
  of variable length. Eg, `scale[x,y,z=1]=...`.
* Pattern matching on records is also useful.
  * As a replacement for keyword parameters: `f{a,b,c=0}=...`.
    If all fields in the record pattern have defaults, then the entire
    pattern also has a default and the parameter itself is optional.
  * As a way of selecting certain fields from a larger record:
    `f{a,b=0,...}=` where the `...` is literal.

Here's a `switch` function that implements multi-way pattern matching,
mapping a list of functions onto a function:
```
cylinder =
  switch[
    {r,h} -> _do_cylinder(r,r,h),
    {r1,r2,h} -> _do_cylinder(r1,r2,h),
    {d,h} -> _do_cylinder(d/2,d/2,h),
    {d1,d2,h} -> _do_cylinder(d1/2,d2/2,h)
  ];
```
All of the functions in the list must have the same number of arguments.

**Special Variables.**
Hmm. Not needed in the Minimum Viable Product.

**Interoperability**
How do we make the Curv function parameter mechanism interoperable
with other programming languages?
* C functions have a fixed # of parameters. This is also relevant for
  generating machine code using LLVM. We have to choose a calling convention,
  typically the C calling convention. So, we don't support varargs.
* C++ additionally supports optional parameters. We can support optional
  parameters, it doesn't break support for C.
* Python and OpenSCAD additionally support keyword parameters.
  JavaScript does *not* support keyword parameters in the same way.
  Instead, by convention you pass an object literal as the final optional
  parameter (defaulting to {}), and this is called the options argument.
  ES 6 supports pattern matching to make this easier:
  eg, `function({x=1,y=2}={})`. What's noteworthy about the Javascript
  calling convention is that each parameter is designated as either positional
  or keyword in the function definition.
* For Curv->other language interop, I think that we don't directly support
  keyword parameters, and instead use the Javascript technique for simulating
  keyword parameters. By convention, there is a final 'options' parameter
  which is a dictionary/record. This way, positional and named parameters
  are strictly segregated. For ->Javascript, it works great, no adaptation
  is required. For ->Python or ->OpenSCAD, the options parameter could be
  expanded to a set of keyword parameters, and the ordering is preserved.
  For ->C++, same, it's expanded to an ordered sequence of optional parameters.
* For other language->Curv interop, there are two cases.
  * First case (C++, Python, Javascript) is that the function is designed
    for use by Curv. Those other languages support lots of types that don't
    make sense in Curv, so Curv callable functions need to be designed as such.
  * Second case is OpenSCAD, we want to be able to call any OpenSCAD
    function or module.

**Function literals:** `atom -> expr` where
```
    atom ::= identifier | parenthesized-formals | bracketed-formals
    parenthesized-formals ::= '(' formals ')'
    bracketed-formals ::= '[' formals ']'
    formals ::= empty | formal-list
    formal-list ::= formal | formal-list ',' formal
    formal ::= identifier | bracketed-formals
```

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

I don't know if this technique is a good idea, but it is possible.

ImplicitCAD has built-in currying (of some form), where if
a required argument is omitted, then a partially applied function is
returned. Eg, `f=max(4); f(5) == max(4,5)`.
That seems too error prone, however.

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
the goals of Curv, and may result in big changes to the language design.

## Predicates
A predicate is a unary function that returns a boolean.
Predicates define a subset of the set of all values.

Predicates are used in the argument to filter, in parameter and
argument assertions.

Curv has 8 basic value types, which partition the set of all values.
Here are the 8 corresponding type predicates:
* is_null
* is_bool
* is_num
* is_str
* is_list(type=is_any,len=null)
* is_fun
* is_record
* is_module

These 8 predicates partition the value space, like the Scheme type predicates
do. They don't all correspond precisely to a set of operations.

I've also written: a type predicate defines a set of values that support
some set of operations. But I haven't defined type predicates that work
this way. For example: Functions and Modules are both callable using
function call notation. So Module should be a subtype of Function.
Records and Modules both support dot notation. So there should be a predicate
for values that support dot notation (I guess; when would you use that?).

Some other possible type predicates:
* is_int
* is_seq -- all sequence values that support len(X), X@i, etc.
  This is complicated by the fact that strings are not first class sequences.
  Really, there is `is_list|is_obj`, and `is_list|is_obj|is_string`.

By Occam's Razor, predicates are better than 'type values' for specifying types.

Some special predicates:
* is_any(p) = true
* is_none(p) = false

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
* filter(is_int & >= 0)
* >=0 & <1
* is_tensor = is_num | is_list(is_tensor)
  * Not quite correct, since it doesn't verify that the array is rectangular.
  * This recursive definition isn't compiled into a thunk,
    because `f|g` compiles to `x->f(x)||g(x)`.

The Haskell prelude has functions for `all o map(P)` and `some o map(P)`.
In Haskell these are `all(P)list` and `any(P)list`.
In Scheme SRFI-1, these are `every(P)list` and `any(P)list`.
Maybe call them all_map and some_map?
