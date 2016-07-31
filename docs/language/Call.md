# Function Call Syntax

There's a variety of function call syntaxes:
* primary: f(x,y)
* primary: f 2, f[x,y], f{x=1,y=2}, f "abc"
* chain: f g x
* f << x, x >> f

There is an inconsistency that bugs me about OpenSCAD2:
* The right argument of a chain-style call is a chain,
  which can't begin with a unary operator (+ or -) or contain infix operators.
* The right argument of `let(x=1)` or `assert(x==1)` is an arbitrary expression.
  This call syntax looks so similar to a chain,
  but the precedence is quite different.

Maybe unify these two syntaxes, so that `let(...)...` and `assert(...)...`
are chains, and you use `let(...) << ...` and `assert(...) << ...`
for the low precedence case.

This opens up the possibility of writing
* `... >> let(...)`
* `... >> require(...)`

```
let(x=1, y=2) << x + y
x + y >> let(x=1, y=2)
```

This simplifies the parser, since `let`, `for`, `assert`, `require`, `echo`
are no longer keywords, but instead builtin entities which aren't values.

## Bindable Meanings that Aren't Values

So the builtin namespace becomes a map from names to meanings
(not names to values).

Definitions like `say = echo;` could be supported.
* A `Module_Expr` contains a map from names to meanings,
  and if the `say` definition is in scope, this is resolved during analysis.
* Can a module value contain a non-value binding?
  I'd expect `use file("say.curv");` to work if the above definition works.
  Note `use` statements are resolved during analysis.
  So, an analysis-time module value can contain a non-value bindable.
