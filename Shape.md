## 9-Jan-2017 GL Compiler
Eliminate most of the boilerplate for vectorized unary numeric functions.
The two parameters are a function and a name.
In all known cases, the Curv name and the GLSL name are identical.
Can test if the name is alphabetic for outputting call strings.
Two cases:
* builtin functions like abs,sin,floor.
  Here we have a Function class with call and gl_call methods.
* the operators +x and -x.
  Here we have an Operation class with eval and gl_eval methods.
Note: can't pass string literals or lambda expressions as template parameters.
Static member functions work as a parameter mechanism.

## Shapes, 2-Jan-2017
Redesign shapes to support: colour, time, 2d/3d polymorphism.

everything and nothing are polymorphic shapes: they are both 2d and 3d.
union is polymorphic: it combines shapes with compatible dimensionality,
whether 2d, 3d or both, reports an error if shapes are incompatible.
union[] returns its identity element, nothing, which must therefore be
polymorphic as well.

Shapes have boolean attributes `is_2d` and `is_3d`
which mark if they are 2d, 3d or both.
* Or use an attribute `dim` which is a boolean vector [Bool,Bool].
  `dim'0` means `is_2d`, `dim'1` means `is_3d`.
  Compute intersection and union using all/any vectorized monoid functions.

The dist function takes ([x,y,z],t) parameters. For 2d shapes, z is 0
and usually ignored. For non-animated shapes, t is ignored.
* 2D shapes need extra code to extract the [x,y] component of the point.
  Eg, dist(p,t) = norm p'[0,1] - r;
  Eg, dist([x,y,z],t) = norm[x,y] - r;

The dist function ought to return 2 results, a dist and a colour.
With inlining and SSA optimization, a big colourless shape tree will end up
with just one colour register initialized to black, and no other expressions
that reference colour. But, how to achieve that with Curv?
* I could hack in support for functions that "return multiple values",
  (which is different from returning a tuple value), similar to how functions
  with multiple arguments (which are not a tuple value) already work.
  We'd need some syntax `(a,b)=f(x)` to receive the values. A function would
  advertise a fixed # of results as part of its identity. There would be slots
  reserved for the return values in the call frame. It's known at compile time
  how many results a given function call expression should return. (a,b) is
  a tuple expression, can occur in limited contexts, like the body of a
  function, or a then/else phrase. And it's also a generator, of course.
  There is an Operator::nvalues field which might be the special value
  INDEFINITE or GENERATOR.
  * The number of values produced by an Operation may be known at compile
    time: 0 is an action, 1 is a traditional expression, (3,17) has 2 values.
    Or it may not be known until run time, in which case it is a generator.
  * So a function can return 0 values. That means it is an action.
    f(x) = echo(x);
  * So, why not permit a function to return an indefinite number of values?
    There's no implementation problem. Rather, it's a style/useability issue.
    I don't want two competing protocols for computing a sequence of values.
* The dist function could return a [d,[r,g,b]] value. Which isn't a GL type.

The bbox has x,y,z,t components. For 2d shapes, the z components are ignored.
For eternal shapes (the common case), tmin is -infinity and tmax is +infinity.

Coloured shapes have a `colour` function that maps [x,y,z,t] to [r,g,b].
This attribute is optional. Union will combine two colourless shapes to
produce a colourless shape. Or combine two coloured shapes.
But when combining coloured with colourless, the colourless shape is
converted to coloured by substituting the default colour.
* `std.default_colour` is the default `default_colour`.
  This can be overridden by defining `default_colour` in the top-level module.
* I guess we can also honour module bindings of `default_colour` in any module
  that is converted to a shape, overriding the value passed in from the parent.
* I will later introduce an environment record, tentatively called `req`,
  which is passed as an argument to `dist` functions during geometry
  compilation. The `default_colour` will be a field of `req`.
* This looks a lot like dynamic binding in OpenSCAD. But without implementing
  that mechanism pervasively in all function calls. The environment is passed
  explicitly. I'll stick with that for now.
* Until the `default_colour` mechanism is implemented, union can report an
  error when combining coloured and colourless shapes. Or we hard code a
  default colour of black [0,0,0].

A simpler design is that all shapes have a geom function: [x,y,z,t]->[d,r,g,b].
Use the simplest design that works: try this, then optimize later.
* A colour field is virtually the same as an infinite shape, except that it
  maps [x,y,z,t]->[r,g,b]. Just use infinite shapes instead.
  Now intersect[colourfield,shape] applies a colourfield to a shape.
* colour(c)shape = make_shape {
    geom p = [shape.geom(p)'0, c'0, c'1, c'2],
    bbox = shape.bbox
  };

I'm worried that this is less efficient, particular for union and intersection.
How does union2(s1,s2) work?
    union2(s1,s2) = shape {
        geom p =
            let (g1 = s1.geom p, g2 = s2.geom p)
            ...
    };
If we can prove that s1 and s2 have the same colour field, then ... is:
    if (g1.d < g2.d) g1 else g2
Otherwise, if we want s1 to dominate s2 where the shapes overlap, then:
    if (g1.d <= 0 || g1.d <= g2.d) g1 else g2
A possible optimization: a shape has an optional "colour cookie" which is
a surrogate for its colour field, and can be compared for equality with other
colour cookies during evaluation, when geom functions are being chosen.
A colour cookie is an [r,g,b] triple, for solid coloured objects. I'm not
sure anything more sophisticated is worthwhile.

# The Geometry Compiler

## 19-Dec-2016 notes, based on first successful shadertoy export.

The Geometry Compiler is currently invoked after evaluation.
* what's good: polymorphism -- the same function can be called with different
  argument types, each call is separately inlined expanded into potentially
  different code (based on argument types).
* what's bad:
  * Code bloat, probably, due to mandatory inline expansion of function calls.
  * Type errors are reported *after* evaluation, meaning no stack trace,
    and therefore can't enter debugger to examine stack. But, maybe we can
    use the CSG tree to provide a different kind of stack trace.
    * There is a stack trace now, and it's fine. It's not an *evaluation* trace,
      but you do see the current CSG stack, so it has similar utility.
  * Better if type errors reported by make_shape constructor.
    * But it's not that bad.

The code bloat is significant. The "assembler" style output has a lot to do with
this. The output is also incomprehensible, which is a barrier to improving
performance. What I would eventually like:
* A SSA style optimizer in Curv. Constant folding and high level optimizations,
  some of which are important for Curv but might not be in your GPU driver,
  like: max[inf,...] -> inf, max[-inf,...] -> max[...].
  * May improve performance due to extra optimizations.
  * Shortens the code and gives you a better mental model for what the cost
    of the code is, for performance tuning.
* Output complex expressions, only use SSA variables when they are multiply
  referenced. Shorter code that is easier to understand and tune.

No constant folding, so -1 becomes
  float r3 = 1;
  float r4 = -r3;
The GLSL compiler should be able to optimize this. But, huge GLSL functions
might create problems.

Fluent math primitives:
* `x^2` compiles into `x*x`
* `exp(x,2)` compiles into `exp2(x)`

If GC happens after evaluation, then potentially the constant folding
and fluent math transformations happen twice, during analysis and during GC.

Maybe the type checking and optimization phase of GC should happen earlier.
* Do as much work as possible during semantic analysis? Maybe we give up on
  JIT constant folding and fluent math transformations. During analysis, we
  recognize the GL subset of Curv, and record type annotations where possible.
  During evaluation, `make_shape` tests if the dist function is GL compatible,
  based on annotations recorded during analysis. This loses some polymorphism
  and probably requires user type annotations to make some code GL compatible.
* Do type checking, at least, maybe some optimization, during evaluation
  in `make_shape` constructor. This preserves polymorphism and eliminates need
  for user type annotations.

Thought experiment: should we compile curv Operations into an abstract GL
operation tree, before generating GLSL code?
* The `make_shape` function compiles the distance function value into GL IR.
  Type errors are reported during evaluation, which is good.
* The GL IR can be optimized, and there can be multiple backends
  for generating GLSL, SPIR-V and LLVM code.
* Introduce another IR? Can't we get by with just a single IR?
  Surely the GL IR is just a subset of the Operation IR.
* Okay, it's the same IR. So this becomes an optimization that maps a
  Closure value's IR and environment onto a new IR tree where environment
  values become compile time constants. We run a constant folding and
  fluent math transformation pass on the resulting IR.
* And, we expose this feature using an `optimize` function that compiles a
  closure into optimized machine code using LLVM. Call this "stupid JIT",
  since we aren't smart enough to do it automatically, and since `optimize`
  only supports a subset of the language.
* Sounds good. Meanwhile, the Operation IR still needs a GLSL code generator,
  which I've already started.

## Older Notes
How to generate GPU code for previewing.

A 2D shape is
    make_shape { dist[x,y]=..., bbox=..., ... }
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
void mainImage( out vec4 fragColour, in vec2 fragCoord )
{
    // transform `fragCoord` from viewport to world coordinates
    float scale = (bbox.w-bbox.y)/iResolution.y;
    float d = mainDist(fragCoord.xy*scale+bbox.xy);
    if (d < 0.0)
        fragColour = vec4(0,0,0,0);
    else {
        vec2 uv = fragCoord.xy / iResolution.xy;
        fragColour = vec4(uv,0.5+0.5*sin(iGlobalTime),1.0);
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
circle(r) = make_shape {
    dist(p) = norm(p) - r,
    bbox = [[-r,-r],[r,r]]
};
square(sz) = make_shape {
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
reporting into the `make_shape` constructor.

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

Basic requirements for a Shape value:
* It's a branded `Ref_Value` with type `ty_shape2d` or `ty_shape3d`.
* getfield("dist") returns a distance Function. class Function now has
  a `gl_call()` virtual function for generating a GLSL call to this function.
* getfield("bbox") returns a bounding box value,
  represented as `[[minx,miny],[maxx,maxy]]` (2D case).

Additional convenience functions, used by builtin Shape classes when
referencing children shapes:
* `double dist(Vec2&, Frame* parent)` calls the dist function.
  Rare, not in the fast path.
* `GL_Value gl_dist(GL_Encoder&, GL_Value)` generates a GLSL call
  to the dist function.
* BBox2& bbox() returns the bounding box with a stronger type.

These could all be implemented by calling getfield().
Or the typed values could be cached in the shape object.

Represent Vec2 and BBox2 as typed wrappers around List type.
* A Vector has exactly the same in-memory representation as a List of numbers,
  (including vtable and virtual functions), but the non-virtual accessor
  functions return doubles, not Values.
* A List* can be bitcast to a Vector* using a function that checks each element
  for validity.
* Vector is a subclass of List that replaces the nonvirtual collection
  access functions, by bitcasting the `array_` data member to `double*`.
* You can create a Vector then upcast it to a List.

----------------------------------------------------------------
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

Currently, `make_shape` maps a record value to a shape.
From the discussion, it would be more useful to use a module value,
and to support module customization on shapes.
(TODO: Module-based shapes once submodules are implemented.)

`curv::Shape2D` is the common superclass of all shape values
(builtin and user defined).
* There is a bbox data member (instead of looking up the bbox using getfield).
  The `make_shape` function checks for the `bbox` field, validates it, and
  stores the data here for rapid access by builtin shapes.
  * We'll see if this is worthwhile.
  * What is the type of bbox? Is it a native Curv value, or must it be
    converted to a curv value (with an extra allocation) by shape.bbox?
* There is a function member, `gl_encode`, for generating GLSL code.

## Generalized Functions

Builtin shape classes must export a dist function value to user space.
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
        GL_Value gl_call(GL_Args) {...}
    };
};
```
Sucks to implement the dist function twice in a builtin shape.
