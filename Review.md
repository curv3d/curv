# Late Feb Review, while implementing stdlib.

## Indexing
`a'i` still looks weird to me. The one syntax I hesitate to show to others
since nobody will guess what it means.
`a[i]` and `{1,2,3}` still seem like the best alternative.
* a[{X,Y,Z}] is vector swizzling.
* `{1,2,3}` is a list
* `{a=1,b=2}` is a record
* `{}` is both the empty list and the empty record.
  * This suggests a set of operations that are polymorphic on records and lists.
    * s[i] -- indexing. i is integer for list, string for record
    * len s -- # of elements of a list or record
    * dom s -- the indexes of s. dom{3,4,5}=={0..2}, dom{a=1,b=2}=={"a","b"}.
    * update(index,newval)struct
  * A possible issue for code that uses type tests and assumes lists and
    records are disjoint. But, this code would need to have different
    semantics for the empty list and the empty record.
* I can keep `a'i` as an alternate syntax.

## Local Definitions and Actions
I now have `let(defs)expr`, `letrec(defs)expr`, and `(act1;act2;expr)`.
Composing the latter with the former is awkward.

[1] `let` and `letrec` support sequential composition within a function body.
It's great. For "sequential assignment", I just do `let(a=1)let(b=a)...`.
I just need a way to insert actions (including conditional actions) into
the sequence. Maybe `do(act1;act2)expr`.
Maybe `while(cond)(body)expr` also fits into this framework.

[2] `(stmt1;stmt2;body)` is also a great sequential composition syntax.
Maybe `let(defs)` and `letrec(defs)` are definitions that can occur as
statements in a semicolon expression? Sequential scoping, same as [1].

```
square(sz) =
  let(sz = if (is_list sz) sz else [sz,sz])
  do(echo("square(",sz,")"))
  make_shape {
    dist p = max(abs p - s/2),
    bbox = [-s/2, s/2],
  };
```
vs
```
square(sz) = (
  sz = if (is_list sz) sz else [sz,sz];
  echo("square($sz)");
  make_shape {
    dist p = max(abs p - s/2),
    bbox = [-s/2, s/2],
  }
);
```
The 2nd syntax is better.
* Replace `let(def1,def2)body` with `(def1,def1)`, for the rare cases where you
  want to do something like `(a=b,b=a)`.
* Replace `letrec(defs)body` with `letrec(defs)`. Maybe introduce `where`.

# Review of Curv after translating `kisrhombille3_7.scad`

[a..b step c] is more readable than [a:c:b]

`if (c) b else a` is more readable than `c ? b : a`, especially multiline nested.

`a*b` in OpenSCAD must be translated to either `a*b` or `dot(a,b)` in Curv.
I might support `a·b` as alias for `dot(a,b)`.
Deciding whether `a*b` is scalar or vector or matrix multiplication is a pain.
Hand translating `*.scad` to `*.curv` is not easy, much more convenient if there
was an OpenSCAD emulator. Can of worms, I know.

a[0] -> a'0
a[x-1] -> a'(x-1)
a.x -> a'X
This is weird. Alternative is:
    a(0)
    a(x-1)
    a(X)
    a[X,Y,Z] // swizzle
    plus confusing reinterpretation of a[i] as vectorized indexing
    at the cost of changing or omitting module/shape customization syntax.
Or:
    a[0], a[x-1], a[X]
    list literal: {1,2,3}
    a[{X,Y,Z}] // swizzle
    Square brackets only used for indexing.
    Funcall and indexing are distinct, customization isn't affected.
    (Defining local variables inside a list literal won't be an option
    as that interferes with record/submodule syntax.)
    {} is both an empty list and an empty record, unless record syntax changes.
        translate{10,20} cube{5,10}
        union{cube(9), sphere(5)}
        union {
            r = 5;
            cube(2*r-1);
            sphere(r);
        }
    In OpenSCAD2, [a,b] and {a;b;} are kinda the same thing, although they have
    different types. This proposal brings those syntaxes closer together:
    {a,b} vs {a;b;}.
    The syntax resembles Lua tables (which unify records and lists).

Translating
    {
        a = ...;
        b = ...;
        modcall();
        modcall();
    }
to
    let (
        a = ...,
        b = ...,
    )
    union [
        modcall(),
        modcall(),
    ]
is awkward.
Remedies:
 1. union [
        a = ...;
        b = ...;
        modcall(),
        modcall(),
    ]
    This one makes perfect sense in the "value script" design.
 2. OpenSCAD2 "geometric object" syntax using {...}. So just use:
      {
        a = ...;
        b = ...;
        modcall();
        modcall();
      }
    Module is a subtype of shape (if all elements are shapes).
    sh_attr(shape) is used to access shape attributes, not dot notation.

Translate
    modcall() for(..) modcall();
to
    modcall() union [for (..) modcall()]

In general, you must distinguish lists from shapes in function arguments,
inserting explicit union where needed to convert.
