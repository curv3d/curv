shape3d maps an object onto a shape, which is a branded object that meets
a set of requirements.

To render a shape S in a preview window, we call S.dist{}, which returns
a distance function, which is translated into GLSL.

There will be native shapes which support this protocol, and user defined:

```
sphere(r) = shape3d {
    name="sphere",      // for CSG file
    param radius=r,     // for CSG file: name and parameters are output
    dist(req)[x,y,z] = sqrt(x^2 + y^2 + z^2),
    bbox = [[-r,-r,-r],[r,r,r]]
};

// or the OpenSCAD2 prototype oriented interface:
sphere = shape3d {
    name="sphere",
    param radius=10,
    dist(req)[x,y,z] = sqrt(x^2 + y^2 + z^2),
    bbox = [[-r,-r,-r],[r,r,r]]
};

sphere;     // prototypical sphere has radius 10
sphere(5);  // a customized sphere
```

A distance function is translated into high performance code,
as an unboxed, strongly typed function that takes a Vec3 argument
and returns a Num. For user-defined distance functions, only a subset
of Curv is supported, and all subexpressions have a compile time
determined type chosed from a small set: Num, Vec3, Bool, maybe others?

There's an API for translating distance functions into GLSL.

## An Implementation Path?

* Learn how to write a simple GLSL script that displays a 3D object.
  Do it in web browser first? Hang out on shadertoy.com, iquilezles.org
  * https://gist.githubusercontent.com/sephirot47/f942b8c252eb7d1b7311/raw/01e17f4dfa6531650ebfb7f2ebe964c5d7112a67/GLSL%2520Raymarch%2520ShaderToy%2520tutorial%2520and%2520example%2520code
  * Use GLSL mouse API to rotate/pan view?
* Build a C++ GL view window that can display a shader script.
* Build a C++ API for building CSG trees that render as GLSL shaders
  in the GL view window.
* Define a Curv Shape3D builtin type, define some builtins that return shapes,
  alter Curv to display shapes in a GL window.

## Compiling Curv into GLSL
To start with, 'dist' will take a vec3 parameter, and the body will compute
a number. No need for type assertions in the user code. The back end will
add an assertion that the parameter has type vec3, and will report errors
about any subexpression that contradicts this. Only a limited set of types
and operations are initially supported. No recursive function calls, so
bottom up type inference can be used. Initially, I don't need conditionals
or loops. If I force the parameter of 'dist' to be a pattern like [x,y,z],
then I can restrict all expresions in the body to have type Num. The only
supported operations will map one or more Num values to a Num.
Plus, `let`.

Later, I'll need more expressive power: conditionals and loops.
GLSL doesn't support recursive functions, but I could allow tail recursion
and compile it into a loop, or just support the 'loop' operator (named let).

For performance reasons, I may want to support vectorized code that can
be compiled into SIMD instructions. A researchy idea is to provide array
language constructs, and compile those into efficient vectorized code.
* Expressive array constructs in an embedded GPU kernel programming language.
