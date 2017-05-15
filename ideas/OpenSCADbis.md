# OpenSCADbis: another proposal for a next generation OpenSCAD

## OpenSCAD2-2017: January 2017
### Goals
**1. Function Values** <br>
Functions are first class values, which can be assigned to variables,
passed as arguments to functions, and returned as results from functions.
Function definitions can be nested, and there is a syntax for anonymous
functions. Function values are lexically scoped closures.

**2. Shape Values** <br>
Shapes are first class values, which can be assigned to variables,
passed as arguments to functions, and returned as results from functions.
The expression syntax is extended so that module calls like
```
    scale([10,20,30]) sphere(10)
    union() { cube(9); sphere(5); }
```
are legal expressions.

**3. No Weird Syntax** <br>
Standard looking syntax is used for defining a function,
calling a function, and passing a function as an argument.
We don't use `@` or sigils or any other non-standard, unfamiliar syntax
for these operations.

**4. 100% Backward Compatibility** <br>

**5. The 3 Namespaces are Not Deprecated** <br>

### Challenges
There are two challenges in achieving the design goals:
the three namespaces, and unintended dynamic scoping in function calls.

OpenSCAD has 3 disjoint namespaces for variables, functions and modules.
In the function call expression `f(x)`, the `f` is looked up in the function
namespace, and the `x` is looked up in the variable namespace.
This creates a challenge if we want to call a function value stored in
a variable `f`, or if we want to pass a function `x` as an argument.

Based on a conversation with Marius, OpenSCAD functions are intended to
be lexically scoped, except for the case of variable names beginning with
`$`, which are dynamically scoped variables. However, there are bugs in the
implementation which cause unintended dynamic scoping for variables that
don't begin with `$`. It's not clear whether these bugs will be fixed, because
the community places a high value on backwards compatibility. So this proposal
cannot depend on these bugs being fixed.

The requirement for function values includes nested functions, and the
ability for one function to construct and return another function.
In order to implement this correctly, inner functions must capture the
values of non-local variables (which may be parameters of an outer function),
and retain the captured values after the inner function is returned as a
result and the lifetime of the captured parameters has ended.
In short, functions must be lexically scoped closures.
This is inconsistent with dynamically scoping.

### Design
Function and module definitions from the original OpenSCAD language
do not change in any way. The functions and modules they define are not values.
Functions defined by old-style function definitions are not values due to
the unintended dynamic scoping bug: as long as this buggy behaviour must be
preserved, they aren't values.

A new style function definition defines the function in the variable,
function and module namespaces.

New style functions are lexically scoped closures. The scoping rules will
need to be defined, but the scope of each non-local variable within a
new-style function is fixed at compile time, before evaluation begins.
Dynamically scoped $ variables are the only exception.

New style functions are values. Old style functions, and modules,
are *not* values.

Most built-in modules are defined as modules in the module namespace
(with no change from before), but they are *also* defined as new style
function values in the function and variable namespaces. (Some, like `for`,
`intersection_for` and `assign`, cannot be represented as functions.)

Most (all?) built-in functions are defined as function values in both the
function and variable namespaces. (Not sure if there are any "funny"
built-in functions for which this will not work.)

In the expression `f(x)`, `f` is looked up in the function namespace,
and `x` is looked up in the variable namespace. The namespace hacks I describe
above make function calls work for new-style functions, and also
allow new-style functions to be used in argument position of a function call.

However, this hack doesn't let you use a function parameter in the
function position of a function call. To make this work, within a new style
function definition, parameter and local variable names are defined in both
the variable and the function namespaces.

### Limitations
This still leaves some edge cases:
* A global variable F containing a function value can't be called from within
  an old-style function definition, or from within a variable definition,
  using the syntax `F(x)`. You have to use `(F)(x)` instead.
* Within a variable definition,
  * You can't call a variable F except using `(F)(x)`.

### Libraries
To avoid the need for namespace prefixes, we need special handling
for libraries (`use` and `include`).

## November 2016
This proposal is a modification of the OpenSCAD2 proposal,
based on a year of public feedback.

What I learned was:
* The OpenSCAD2 proposal is disliked by some members of the community
  because it deprecates 3 namespaces in favour of 1 namespace.
  Some people have adopted programming idioms that require 3 namespaces,
  and don't want to give up those idioms. They also don't want to be
  locked out of new language features.
* Other members of the community want the ability to write their
  new code using a single namespace, and don't want to be forced to
  continue dealing with 3 namespaces.
* The OpenSCAD2 design is not compatible with some old code in Thingiverse.
  * `write.scad` relies on a bizzare bug/feature where labeled arguments
    passed to a function or module, if they aren't the names of declared
    parameters, then they are dynamically scoped, even if their names
    do not begin with `$`. Talking to Marius, this is a bug, but some people
    consider it a feature, and then there is also backwards compatibility
    to consider.
  * `relativity.scad` relies on the fact that the children of a module
    are passed by name, instead of by value. Each time you call `children()`,
    the children are re-evaluated, and you can get a different result
    each time if you specify different values of dynamically scoped
    `$` parameters.

OpenSCAD is a strange and buggy mixture of lexical and dynamic scoping.
It is *intended* to be lexically scoped, except for `$` variables,
but dynamic scoping surfaces in various edge conditions. It's possible that
we will fix all of these bugs, but it's also possible that we might decide
we can't fix them due to backwards compatibility. And the discussion has
been dragging on for years. One of the goals of OpenSCADbis is to provide
a plan for moving forward that works even if these bugs aren't fixed.
So we decouple the issue of making non-backward compatible bug fixes
from the issue of adding the new features in OpenSCADbis.

One of the features of OpenSCADbis is first class function values.
I think it is impossible to implement this feature correctly while still
maintaining backward compatibility with the legacy scoping rules
of OpenSCAD functions. So there will have to be two kinds of functions.
* The old kind are written using the old function definition syntax, and
  this proposal doesn't force their semantics to change (although that
  can still happen independently on a different time frame).
  The old functions are *not first class values*.
  The old functions exist in the function namespace.
* The new kind of functions are either anonymous function literals,
  or they are written using the new function definition syntax
  taken from OpenSCAD2. The new functions are lexically scoped, except
  for dynamically scoped `$` variables. The new functions exist
  in the variable namespace, and they are values.

In OpenSCADbis, old and new functions can coexist in the same script,
which is a big change from OpenSCAD2.

Inside of a new function definition, you exist in the pure blissful world
of a single namespace. Scoping rules are simple and make sense.
Is this even possible? How does it work? Within a new function,
* All local definitions exist in the variable namespace.
* The syntax `f(x)` looks up `f` in the variable namespace.
* Built-in functions like `sin` exist in the variable namespace.
  Actually, this could be a global change: `sin` exists everywhere
  in both the function and the variable namespaces.

Ditto for modules. Old style module definitions exist in the module
namespace, and are not values. New style module definitions are actually
new style function definitions, there's no difference, as per OpenSCAD2.

With this design, how do we interpret function/module calls that are
outside a new function definition? The same way as always. A new function
definition puts its name into all 3 namespaces, and that's what makes it
accessible to code that uses the old scoping rules.

With this design, how does a new function access old-style functions and
modules?
* Maybe, using namespace prefixes: `f$foo` and `m$foo`.
  Obviously there's no other choice if there is also a variable named `foo`.
* It would be nice if there was a way to eliminate the need for namespace
  prefixes in cases where there is no ambiguity. For future research.

Suppose you write an old style function or module, and you want to accept
a function value as an argument. How do you call that function?
Using a namespace prefix, `v$arg(x)`.
