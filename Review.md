# Review of Curv after translating `kisrhombille3_7.scad`

[a..b step c] is nicer than [a:c:b]

`if (c) b else a` is nicer than `c ? b : a`, especially multiline nested.

Deciding whether a*b is scalar or vector or matrix multiplication is a pain.
Hand translating *.scad to *.curv is not easy, much more convenient if there
was an OpenSCAD emulator. Can of worms, I know.

a[0] -> a'0
a[x-1] -> a'(x-1)
a.x -> a'X
This is weird. Alternative is:
    a(0)
    a(x-1)
    a(X)
    plus confusing reinterpretation of a[i]
at the cost of changing or omitting module/shape customization syntax.

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
 2. OpenSCAD2 "geometric object" syntax using {...}.
    But I don't know the semantics. If {..;..;} is both a module *and* an
    implicit union of shapes, what fields are exported?
    * Maybe a submodule is not a shape, you need explicit conversion
      via union {...} or emshapen {...} or whatever, same as explicit conversion
      of a list to a shape. But now submodules are not a syntactic shorthand.

Translate
    modcall() for(..) modcall();
to
    modcall() union [for (..) modcall()]

In general, you must distinguish lists from shapes in function arguments,
inserting explicit union where needed to convert.
