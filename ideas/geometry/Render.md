Once you go down this path, there is a subtle issue that arises, involving
`$fn`. Certain circular primitives, like circle(), sphere() and cylinder(),
use special variables like $fn to determine the fineness of the mesh when
approximating a circle. The design issue is whether $fn should be late bound or
early bound. The problem arises once you add shape values and shape variables,
which don't exist in OpenSCAD, but do exist in my language.

In OpenSCAD, we can say that $fn is late bound. Suppose I define a module
M which takes a shape as an argument:
    M() sphere(5);
In OpenSCAD, the value of $fn is not bound at the time that the module
is called. Instead, $fn is bound each time M references its children.
For example (try it),
    module M() {
        $fn=1; children();
        translate([10, 0], $fn = 5) children();
        translate([20,0], $fn = 10) children();
    }
    M() sphere(5);

By contrast, in my language, shapes are values. You can pass a shape as an
argument to a function, like this:
    M(sphere(5))
Function arguments are fully evaluated before the function is entered, just
like in most languages. If OpenSCAD were extended to work the same way, then
does that mean $fn is early bound instead of late bound? It's not exactly a
backwards compatibility issue, because in "proposal #3", modules would continue
to exist and have the same semantics. The question is about the semantics
of a new feature, shape values being passed as arguments to functions.

Here's another way of thinking about it. Are the following two programs
the same or different?
 1.
    M() sphere(5);
 2.
    s = sphere(5);
    M() s;

In Curv, fineness parameters for mesh rendering are late bound.
If a sphere is converted to a polyhedron, then the fineness/error tolerance
is not decided when the sphere is constructed during evaluation time, but
much later. If you explicitly invoke the Curv analog of `render()`
(to polygonize a shape), then the fineness is only bound then.

AS A CONSEQUENCE, cylinder() and prism() are different primitives.
For cylinder(), $fn is late bound, while for prism(), the number of sides
is determined when the shape is constructed.
