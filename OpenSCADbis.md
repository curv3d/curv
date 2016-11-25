# OpenSCADbis: another proposal for a next generation OpenSCAD

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
