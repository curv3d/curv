# Review of Curv after translating `kisrhombille3_7.scad`

[a..b step c] is nicer than [a:c:b]

`if (c) b else a` is nicer than `c ? b : a`, especially multiline nested.

Deciding whether a*b is scalar or vector or matrix multiplication is a pain.
Hand translating *.scad to *.curv is not easy, much more convenient if there
was an OpenSCAD emulator. Can of worms, I know.
    I'll add a +* b to compute dot and matrix product.

a[0] -> a'0
a[x-1] -> a'(x-1)
a.x -> a'X
This is weird.

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

Translate
    modcall() for(..) modcall();
to
    modcall() union [for (..) modcall()]
