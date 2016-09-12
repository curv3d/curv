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

So the issue is prefix keyword operators: can they use the same syntax as
function calls?

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
(`use` is a prefix keyword operator which can't be builtin due to scoping
issues.)

There's still an inconsistency. `if(cond)stmt` has different precedence
from the other special operations. Maybe `if` expressions are chains,
and the 2nd and 3rd arguments are chains.
```
if (x > y) x else y + 1
<=> (if (x > y) x else y) + 1
```
Extending this `if` to support `<<` is questionable: the syntax will be
quite alien to C/OpenSCAD users.

So I return to my original position.
* Special operations: `let`, `for`, `assert`, `require`, `echo`, `if`
* Special operations can't be function values, due to syntax and/or semantics:
  * `let` and `for` are name-binding.
  * `assert` and `echo` can be invoked as `echo(x)y` or as `echo(x)`.
  * `if(c)a` looks functional but `if(c)a else b` is special syntax.
* They are reserved words, known by the parser. Maybe the parser is
  parameterized by a special-ops table, to get the benefits that would otherwise
  be gained by representing special-ops as builtin bindings.

## Generalized Chains
In Rust and Perl, the `if` has mandatory braces around the consequent and
alternate. This eliminates the style debate over whether to use braces,
and eliminates the dangling `else` ambiguity.

In Curv, the corresponding decision is to model `if` expressions as chains.
Same with let, for, echo, assert, etc. The only context where we greedily
consume an entire *expr* as an argument is in low-precedence right associative
infix operators like `=` and `->`, which also have symbolic names.

So,
* if (cond) nprimary
* if (cond) chain
* if (cond) nprimary else nprimary
* if (cond) nprimary else chain

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
