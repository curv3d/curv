How to generate GPU code for previewing.

A 2D shape is
    shape2d { dist[x,y]=..., bbox=..., ... }
The dist and bbox fields are used by the GPU previewer.

We need to assemble a main_dist function, which is then called by the
fixed rendering code.

To begin with, all primitive shapes are hard coded.
square translate rotate

So what is the GLSL compiled representation?
Make it look like HyperFun code.

square(vec2) -> d
translate(delta, d) -> d
rotate(angle, d) -> d

// how to translate a shape
float translate(vec2 delta, shape, vec2 pt)
    shape.dist(pt + delta)

Internal representation of shape values.
There is a Shape class <: Ref_Value.
And subclasses for each primitive. Gives us a CSG tree.

Compile the CSG tree into nested function calls, as above.

ShaderToy circle, [0,0] is bottom left:
---
float circle(float r, vec2 pt)
{
    return length(pt) - r;
}
float mainDist(vec2 pt)
{
    return circle(10.0, pt);
}
// minX, minY, maxX, maxY
const vec4 bbox = vec4(-10.0,-10.0,+10.0,+10.0);
void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
    // transform `fragCoord` from viewport to world coordinates
    float scale = (bbox.w-bbox.y)/iResolution.y;
    float d = mainDist(fragCoord.xy*scale+bbox.xy);
    if (d < 0.0)
        fragColor = vec4(0,0,0,0);
    else {
        vec2 uv = fragCoord.xy / iResolution.xy;
        fragColor = vec4(uv,0.5+0.5*sin(iGlobalTime),1.0);
    }
}
---

ShaderToy code generation:
* output standard prelude (distance functions for all primitives),
* then output float mainDist(vec2 pt) {...}
  which contains CSG tree rendered as nested function calls.
* then output bounding box as eg `const vec4 bbox2 = vec4(1,2,3,4);`
* then output mainImage

## How to compile a shape closure to GLSL
* GLSL has no function pointers, no recursion.
  Hard to see a simple mapping from Curv code.
* Inline expand each distance function call and generate SSA code.
* There are tight restrictions on what can occur in a distance function.
  Can't call user defined functions, only a limited set of builtin operations.
  All expressions are statically typed, with a restricted set of types:
  Number, Vec2, Vec3, Vec4, Boolean.

## GLSL Code Generator
We output a series of SSA triples, like
```
r0 = r1 + r2;
r3 = sin(r4);
```
It's very close to SPIR-V, or LLVM IR.

This suggests, in the future, using an SSA or equivalent IR (CPS) on which
optimizations can be performed. Maybe these optimizations speed up the GLSL.

Right now, we'll generate the SSA-like GLSL from the existing Operation tree
representation, with no optimization. (Simplest code possible.)

There is a function that outputs a Shape as GLSL (as the body of mainDist).
```
circle(r) = shape2d {
    dist(p) = norm(p) - r,
    bbox = [[-r,-r],[r,r]]
};
square(sz) = shape2d {
  dist[x,y] = max[abs(x-sz.x/2), abs(y-sz.y/2)],
  dist(p) = max abs(p - sz/2),
  bbox=[ [0,0,0], sz ]
};
```

A Shape2D value is a wrapper around a record value.
Supports the `.` operator, and any future record operations.

A GLSL_Coder is a stateful object that converts the body of a dist function
to a sequence of GLSL triples.
Parameters, locals and nonlocals are converted into GLSL variables
on first reference, then the variable name is used on subsequent references.

GLSL_Value is an abstract value, which expands to a variable name in generated
code (similar to the LLVM IR API).

We need to assign a static type to each Expression in a dist body.
Supported types are Number, Vec2, Vec3, Vec4, Boolean.
The dist function argument has type Vec2.
The result must be a Number.
The supported operations are monomorphic.

Type inference is performed during GLSL code generation, which happens
*after* evaluation (since we are operating on a Closure value).
It is strictly monomorphic and computed bottom up.
For the future, I've considered doing type inference during analysis,
and marking each Expression with a type. But that's not the plan right now.
Here we are operating on a Closure value, not strictly just an Expression tree,
and we'll be inferring types from dynamically computed nonlocal values.
Different types can be inferred within the same Lambda for different closures,
especially once we support array broadcasting. Eg, this happens with square(5)
vs square[5,10], using the simplest definition of square (with broadcasting).

We should not generate bad GLSL. We should report all type errors in the
code generator. The Lambda contains context for reporting errors, but we
won't have a stack trace. This could be mitigated later by moving error
reporting into the `shape2d` constructor.

Only a small subset of operations is supported by the GLSL code generator.
Maybe `virtual void Operation::glsl_code(GLSL_Coder&)`?
Throws exception if not implemented.

Some supported operations are builtin functions like `sqrt` and `sin`.
Where does the `glsl_code` function reside?
* Implement `sqrt` etc as metafunctions: `Sqrt_Call::glsl_code`.
* Add the glsl API to `Function`. This also works if `sqrt` is passed as an
  argument value and is dropped into a Closure environment by that route.
* See which approach requires the least code.
* Note, I've considered metafunctions as a way to do peephole optimization
  on builtin function calls, eg `log(x,2)` -> `log2(x)`, which is also relevant
  to GLSL code generation (which has log2 but not log(x,b)).

GLSL_Value
Add_Expr::glsl_encode(GLSL_Encoder& gl)
const
{
    auto val0 = gl.encode(*arg0, GL_NUMBER);
    auto val1 = gl.encode(*arg1, GL_NUMBER);
    auto result = gl.new_value(GL_NUMBER);
    gl.out << result << "=" << val0 << "+" << val1 << ";\n";
    return result;
}

## Shape Representation

A shape is a set of named fields. The `bbox` and `dist` functions
are mandatory, some shapes have additional fields according to various
protocols yet to be defined. The fields can be accessed using dot notation.

Shapes have a *module* nature, not a *record* nature. It doesn't make sense
to use `merge` to add new fields or especially to modify existing fields,
since the `dist` function is closed over the original field values.
And built-in shape classes don't support field extension.
But we can support OpenSCAD2-style customization to update existing
parameter fields. Adding new fields isn't supported, unless we implement
OpenSCAD2 object extension, but you can use modules to encapsulate
a set of shapes with a set of parameters.
* For user defined shapes, the module representation may be more efficient
  if many instances of a shape type are created, since the Dictionary is shared.

Is equality supported for shape values?
* Yes, it makes sense. Two shapes with the same type are equal if their
  parameters are equal. (But "same type" isn't well defined, since we don't
  have user-defined types.)
* No, shapes are like modules, and modules don't support equality since they
  are "code values".
* We'd need some analog of user-defined types, some kind of "branded values",
  to do this properly. Until then, shapes do not support equality.

User defined shapes have a dictionary for looking up field values.
Builtin shape classes will store their fields in C++ class data members.
Both kinds of value implement the same Curv API for field access:
    `virtual Value Ref_Value::getfield(Atom) const`
which returns the missing value if the field isn't defined.
This allows builtin shapes to access fields of child shapes in a uniform way,
and provides a more flexible Pythonic value API.

I also considered:
* There is a generic Dictionary data field that can somehow be used to
  look up fields in arbitrary data structures. A hash-tree mapping names
  onto field accessors. A field accessor is either an immediate value,
  or an index into the object's slot array, or maybe it could also be a
  function that maps the object to a value. We'll need to switch on the
  accessor tag. So it needs a dictionary lookup and a switch, just like the
  current `Dot_Expr` implementation: probably the same speed.
  More complex and more restrictive than the virtual function.

There also needs to be an API for iterating over fields.
A field iterator contains an (Atom,Value) pair, or is null.
Either there is a Field_Iterator type (more difficult), or there is
`each_field(function<void(Field)>)` (easier).

A general question about fields in Curv. Two approaches:
1. Every value contains zero or more fields, accessed using dot notation.
   `fields(42)` returns `[]`.
2. Only certain value types support fields.
   `fields(42)` is an error.
Option 2 seems more Curv-like. Only Record, Module and Shape have fields.
So should they have a more specific superclass than Ref_Value?

Currently, `shape2d` maps a record value to a shape.
From the discussion, it would be more useful to use a module value,
and to support module customization on shapes.
(TODO: Module-based shapes once submodules are implemented.)

`curv::Shape2D` is the common superclass of all shape values
(builtin and user defined).
* There is a bbox data member (instead of looking up the bbox using getfield).
  The `shape2d` function checks for the `bbox` field, validates it, and
  stores the data here for rapid access by builtin shapes.
  * We'll see if this is worthwhile.
  * What is the type of bbox? Is it a native Curv value, or must it be
    converted to a curv value (with an extra allocation) by shape.bbox?
* There is a function member, `gl_encode`, for generating GLSL code.

## Generalized Functions

Builtin shape classes must export a dist function value to user space.
But, no current support for builtin closures (C function + environment).
* Generalize curv::Function to contain an environment pointer,
  as `Shared<Ref_Value> env_`, passed as arg to `function_`.
  Maybe use a template, which reduces to a monomorphic type in `Call_Expr`.
* `virtual Value GFun::call(Frame&)`.
  Adds an extra indirection for native function calls.
  But, it's idiomatic C++.
* Also, provide a generic GFun interface to Function and Closure.

```
Value
builtin_len(Frame& args)
{
    auto& list {arg_to_list(args[0], At_Arg(0, &args))};
    return {double(list.size())};
}
{"len", make<Builtin_Value>(1, builtin_len)},
-->
class Std_Len : public Function
{
    Std_Len() : Function(1) {}  // nargs
    Value call(Frame& args)
    {
        auto& list {arg_to_list(args[0], At_Arg(0, &args))};
        return {double(list.size())};
    }
};
{"len", make<Builtin_Value>(make<Std_Len>())},
```

```
class Square : public Shape2D
{
    double size;
    Value getfield(Atom a)
    {
        if (a == "dist") return make<Square_Dist>(this);
        ...
    }
    class Square_Dist : public Function
    {
        Shared<Square> self;
        Value call(Frame& f)
        {
            pt = get_vec2_arg(0, f);
            ...
        }
    };
    void gl_encode(GL_Encoder& gl) {...}
};
```
Sucks to implement the dist function twice in a builtin shape.
