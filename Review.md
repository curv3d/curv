# Review of Curv after translating `kisrhombille3_7.scad`

[a..b step c] is more readable than [a:c:b]

`if (c) b else a` is more readable than `c ? b : a`, especially multiline nested.

`a*b` in OpenSCAD must be translated to either `a*b` or `dot(a,b)` in Curv.
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
    plus confusing reinterpretation of a[i] as vectorized indexing
    at the cost of changing or omitting module/shape customization syntax.
Or:
    a[0], a[x-1], a[X]
    list literal: {1,2,3}
    Square brackets only used for indexing.
    Vectorized indexing is a[{1,2,3}].
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
