# Dynamic Scoping

TeaCAD supports dynamically scoped variables.
If a variable's name begins with `$`, then it is dynamically scoped,
otherwise it is lexically scoped.

## What Does That Mean?
A TeaCAD script file has an associated tree of lexical scopes.
The root scope is the *builtin* scope, which contains the variables
that are built in to the language. Under that is the file scope, and under
that are smaller nested scopes that are introduced by certain syntactic
constructs: `let`, `{...}` and function literals.

When a variable is lexically scoped, the system looks in the smallest
enclosing lexical scope for its value. If it isn't found, then it searches
the next smallest enclosing scope, and so on up the tree until the root
(builtin) scope is searched.

When a variable is dynamically scoped, the enclosing lexical scopes are
searched until a function literal is reached. The search resumes at the site
where the function was called, and then the lexical scopes at that site
are searched until another function literal is reached, and so on up the
call stack. When the call stack is exhausted, the lexical scopes surrounding
the outermost function call are searched up to the *builtin* scope.

This picture of dynamic scoping corresponds to an implementation called
deep binding. There is another implementation called shallow binding, in
which there is a global table containing the current value of every
dynamic variable. When `let ($foo=a) expr` is evaluated, the current value
of `$foo` is pushed on a stack, then the value `a` is stored in the table
entry for `$foo`, then `expr` is evaluated, then the old value of `$foo`
is popped off the stack.

These two implementations, deep binding and shallow binding, are equivalent.
Do not be confused by the fact that the shallow binding implementation
associates a mutable storage cell with each dynamic variable. This does not
mean that TeaCAD is an imperative language with mutable global variables.
The TeaCAD language specification is not biased towards one particular
implementation.

## Why Is This Useful?
Dynamically scoped variables are used as implicit function arguments,
in those cases where it makes sense to specify the value of a particular
argument for all function calls in a particular call tree. This avoids the
need to explicitly pass these arguments through every intervening function
call up to the functions that need it.

## Centralized Definitions and Default Values
A lexically scoped variable is defined at a particular spot in the source
code. It is possible for two different libraries to export lexical variables
of the same name; they can be kept distinct, and disambiguated
in various ways, such as dot notation (libname.foo).

A dynamically scoped variable does *not* have a unique point of definition
in the source code. All dynamic variables share a single global namespace.
This creates the possibility for two different libraries to use dynamic
variables with the same name but conflicting meanings. This is mitigated by
the fact that new dynamic variables are rarely needed. There will never be
a large number of them, so it's easier to avoid name conflicts.
Still, it might be wise for user-defined libraries to use a naming convention
that avoids conflict with other libraries: something like $*libname*_*name*
would work.

If there is a reference to an undefined lexical variable, then it is
reported as an error at compile time, before the script is evaluated.

If there is a reference to an undefined dynamic variable, then it is not
detected until run time. There's no way to specify a default value for a
dynamic variable, since they have no central point of definition. Therefore,
an undefined dynamic variable defaults to the value `null` (except in the case
of builtin dynamic variables, which may have a different default).
Application code will typically test if a dynamic variable is `null`, and
substitute an appropriate default.

An object or library cannot export a dynamic variable.
Dot notation does *not* work for dynamic variables: `foo.$bar` is not supported.
Um, except that you can qualify builtin dynamic variables using `builtin.`
to get their default values, for example using
`builtin.$seed` to get the default value of `$seed`.

## Setting a Dynamic Variable
Three ways to set a value for a dynamic variable:
* `let ($foo=x) ...`
* `f(..., $foo=x)`
* `{$foo=x; ...}` and also at the top level of a script file.

A formal parameter of a function cannot have a name beginning with `$`.

Dynamic variable definitions only affect function calls in the scope of the
definition. Their scope does not extend inside of function literals or
function definitions. This last bit might be counterintuitive, but it follows
directly from the definition of dynamic variables. For example,
```
M = {
  $fn=32;
  circ(r) = circle(r);
};
M.circ(10);
```
Within the body of object M, the definition of `$fn=32;` has no effect,
since it cannot be seen inside the definition of `circ`.




# Previous Draft
The general idea is this: When a variable is lexically scoped, the system looks
to where the function is defined to find the value for a free variable. When
a variable is dynamically scoped, the system looks to where the function is
called to find the value for the free variable.

The c2.com wiki says:
* In lexical scoping, you search in the local function (the function which is
  running now), then you search in the function (or scope) in which that
  function was defined, then you search in the function (scope) in which that
  function was defined, and so forth.
* In dynamic scoping, you search in the local function first, then
  you search in the function that called the local function, then you search
  in the function that called that function, and so on, up the call stack.

TeaCAD supports dynamic scoping for special variables beginning with '$'.
It's for compatibility with OpenSCAD, but it's also useful.
The semantics need to follow from the use cases.
I'm not sure that the OpenSCAD semantics are necessarily the correct ones,
so I'd like to independently work out the correct semantics.

One use case is my design for functional random numbers, the `random` function
which has a `$seed` parameter which defaults to a dynamically scoped variable.

The $fn etc variables for controlling the resolution of circular mesh
approximations seems to require the same mechanism as $seed.

Another example of dynamic scoping from OpenSCAD is `relativity.scad`.
In this case, a module can set dynamically scoped variables that are visible
in their `children` arguments. Part of the deal is that `children()`
re-evaluates the tail argument every time it is called, and special variables
can have different values on different calls. I need this for F-Rep, but
I think a different mechanism is needed than this, since I don't necessary
agree with call-by-name argument passing.

## How It Works
A special variable is one whose name begins with `$`. Special variables are
dynamically scoped, as defined here.

In TeaCAD, it's illegal to reference a variable that isn't defined within enclosing
lexical scope. That also applies to special variables. The definition given by
lexical scoping is the default value if it isn't overridden by the dynamic
scoping mechanism.

Note that two scripts could define distinct special variables with the same
name, but different default values. At least, I hope this works, I hope we
aren't forcing all special variables to share a single global namespace.

The basic idea, I think, is that within the body of a function, we lookup
a special variable by first checking if the variable is defined by the caller's
context: first, was a $foo= parameter passed, second, is there a definition
in the scope surrounding the call. If that fails, then we check the scope
surrounding the function body.

How does this compare to special variables in Lisp?
* All special variables belong to a single global namespace. There is a single
  cell holding its current value.
* (let (*foo* value) ...) pushes the old value of *foo*, sets *foo* to value,
  evaluates ..., then pops *foo* to restore its original value.

Scheme has lexically scoped variables, and `fluid-let`. Variables are not
marked special, unlike in Lisp. This does make it possible to have module
specific 'special' variable bindings, without name collisions.
Eg, `fluid_let(M.$foo = 42) ...` in TeaCAD.

### idea: fluid-let
Here's an idea:
* `let ($foo=x) ...` is the equivalent of fluid-let in Scheme. `$foo` must
  be defined in the lexical scope. Also, `let (M.$foo=x)...` is legal.
* `f(x,$foo=y)` is equivalent to `let($foo=y)f(x)`.
* `$foo=x;` in a script just defines a new special variable and places it in
  the lexical scope.

This permits libraries to define their own special variables, without
namespace conflicts with other libraries.

However, objects are first class immutable values, and `let (obj.$foo=x)`
mutates an object. To make sense of this, we would need to provide an
account of object identity. Yikes.
To fix this,
* introduce a distinction between modules/namespaces, which are not values,
  eg M::x, and objects, which are values, eg obj.x
* or, all special variables share a single global namespace.

Also, the traditional use of `{$fn=20; ...}` would not work.

### idea: single global namespace
All special variables share a single global namespace.

Is there some way to globally define a default value?
What if there are multiple conflicting definitions?
Maybe it's best to say that the default value is always `null`.
* default value of $seed is not `null`

`let ($foo=x) ...` is the equivalent of fluid-let in Scheme.

`f(x,$foo=y)` is equivalent to `let($foo=y)f(x)`.

`$foo=x;` in a script is like wrapping each top level expression in the
script (including definiens) with `let($foo=x)`.

So that's an unambiguous definition of the semantics in terms of the
fluid-let construct in Scheme, which is the simplest dynamic scoping mechanism.

{$fn=20; circle(10);} will be equivalent to circle(10,$fn=20).

In obj={$fn=20; foo(x)=circle(x);} the $fn= has no effect.
In a call to obj.foo(), the value of $fn will come from the calling context,
and not the defining context. Unless you include obj and then call
foo() from the same context. Assuming that `include` will include special
variable bindings.

Special variables expose evaluation order?
* In a pure functional language, there shouldn't be a difference between
  foo= and foo()=, in terms of the result returned by foo or foo().
  But here, the foo() version is sensitive to special variables, which are
  extra implicit parameters, whereas I don't think we want foo= to have these
  semantics (by-name binding).
* What does this mean: let($foo=0)(function-literal)?
  Function literals do not capture special variable bindings.
* What does this mean: let($foo=0)(object-literal)?
  * An object is a list with metadata, so we expect that subexpressions of an
    object literal capture special variables in the same cases
    as `let($foo=0)[list-literal]`.
  * Lazy evaluation is okay, as long as it doesn't change semantics.
    A lazy (deferred) expression should capture special variable bindings,
    just as non-local bindings must be captured.

### incremental update of special variables
Can you set a special variable as a function of its previous value?

A use case: random numbers in animations.
Arguably the default $seed should not change from one step of an animation
to the next. But then, if you need randomization across animation steps,
then you may wish to use something like `$seed = random[$seed,$t]`.

Let's say that `$outer` is the parent scope, and is magic syntax
that can only be used in the context of `$outer.foo`.
Then you'd write:
```
$seed = random[$outer.$seed, $t];
```
