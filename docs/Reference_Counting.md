# Recursion and Reference Counting
I can use reference counting to manage storage,
even in the presence of first-class recursive function closures.
Benefits:
* easy C++ integration
* 'mutable' data structures: upd(array,index,newval) returns a copy of array
  with one element updated, and can be used to implement constant-time
  array update algorithms. Implementation: modify array in place if refcount==1.

Recursive definitions are legal in object literals.

Within a script, a function definition binds a name to an rfunction.
An rfunction is not a closure or even a value: it is just machine code
for the function, with an implicit environment parameter E which contains
non-local bindings.

If F or obj.F is bound to an rfunction, then F(X) or obj.F(X) invokes
the rfunction, passing either the static environment of F or obj as the
environment argument. If F or obj.F is used outside the context of a function
call, then it is converted to a closure value containing the environment
and the rfunction.

The important thing is that if an object contains a function definition F,
then the object value doesn't contain a reference to the closure for F,
because that would create a reference loop.

If a script contains a definition whose definiens isn't a function literal,
but does contain recursive references, then the definiens is packaged inside
an rthunk, which is structurally an rfunction with no parameters other than
the environment param. Each time this definition is referenced, the rthunk is
called. That's not very efficient, but it satisfies the requirement of not
creating a cycle in the refcount graph. It permits a lot of the same recursive
definitions as can be expressed in Haskell, beyond what's permitted by an
imperative language.
* Could speed up an rthunk by partially evaluating it w.r.t. the environment,
  storing the partially evaluated thunk back into the parent object.
  This makes sense if it is being called many times.
* I can imagine a partially-evaluating tree evaluator that uses copy-on-write
  to update branches of the tree with results.

let() bound definitions also use recursive scope.
let(a=X,b=Y)Z can be rewritten as {a=X;b=Y;_=Z}._
so no additional implementation mechanisms are strictly required.

Note if you want sequential assignment, you use let(a=X)let(b=Y)Z.

In summary, there are no significant restrictions on recursive definitions,
and refcounting cycles are avoided.

## Older Stuff
I need to prove that ref-counting will work.
My initial assumption is: no infinite/recursive data structures.
But, I support recursive functions and closures. So that's the problem for
which I need a non-recursive representation.

## Function Definitions

The basic idea is similar to how the Y-combinator eliminates recursion.
A potentially recursive function (prfun) is passed an additional object argument
containing the environment, and recursive calls are performed via this
environment argument.

Recursive definitions are only legal within object literals (aka scripts).
Within a script, a function definition binds a name to a prfun.

A prfun is a bindable entity, but isn't a value.
The expression E.F(X) checks if F is a prfun, and if so, invokes F with
the argument list and the implicit environment argument E.

If E.F is used outside the context of a function call, then it is converted
to a closure value containing E and F.

## Function Literals

Now add function literals. Eg,
```
f = identity(x->f(x));
g = [x->g@0(x)];
```
A conventional representation will cause cycles. The literal will contain
a direct or indirect pointer to f, the top level object will contain a
pointer to the literal.

I could:
- Forbid this kind of recursion, and only permit recursive loops among
  function definitions.
- Permit this kind of recursion, and find a way to represent the resulting
  values using refcounting.

Maybe the function literal and the top level object both point to some
third object... which contains?

weak pointers?

When a function value is stored in an object, we attempt to strip it down
to a prfun to prevent cycles?

## Recursive Definitions

Scripts use 'recursive scope' (as in letrec), but only functions can be defined
recursively. No recursive objects or lists. JIT compiler enforces this.
Also, no recursion between script files.

More generally, recursive references only permitted in function literals.
That loosens things up to permit the following:
```
f = identity(x->f(x));
g = [x->g@0(x)];
```

If two values have a recursive reference loop, then refcounting fails.
Unless we join them into a single object with a single refcount.
Or use refcount indirection so that two value objects share a single
refcount object. This would have to be set up at compile time, after
analyzing the reference graph within a script.

This sounds hard. I don't have a good plan to support recursion between
functions, lists and objects. Let's impose restrictions:
- recursive references only permitted within function literals
- recursive references must refer to a function definition

What about this:
```
f(x) =
  { g(x) = f(x) }
  ;
```
f is a prfun with an implicit environment argument E.
The recursive call f(x) is equivalent to E.f(x).
The inner object literal is a closure that captures E.
The inner object value will contain a reference to E, which is the top level
object, but not a reference to f. I see no cyclic references.
to f.
