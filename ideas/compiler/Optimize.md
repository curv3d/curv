# Byte Code Optimizations

## Lifting Invariant Expressions
If a complex subexpression has the same value each time through a loop,
it should be lifted out of the loop and evaluated once. Or, the more general
equivalent of this for a functional language.

In Curv:
* lifted out of a for loop (subexpression doesn't depend on a loop variable,
  transitively)
* lifted out of a function (subexpression doesn't depend on a parameter,
  transitively)

I imagine this works by assigning a 'level' to each identifier and constant.
The level of a compound expression is the max of its components.
* Level 0 is a builtin binding or compile time constant.
* Level 1 is a top level `param`.
* A function parameter has a higher level than the identifiers in its parent
  lexical scope.
* A `for` variable has a higher level than the identifiers in its parent scope.

If a function call of level `N` has an argument of level `<N`, that argument
can be lifted.

## Static Expressions
As an optimization, aka constant folding.
This is a special case of lifting invariant expressions, where the
invariant subexpression is lifted all the way up to compile time.

An expression is static if all of its free variables are static:
* Builtin bindings like `true` and `sqrt` are static.
* A function formal parameter is not static.
* A `let` bound variable is static if its definiens is static.
* A parameter binding of a module is not static.
* A non-parameter binding of a module is static if its definiens is static.
* In the case of recursive definitions within a `let` or module,
  we tentatively assume that all non-parameter bindings are static,
  then attempt to disprove this by finding non-static free variables
  in the definientia. If no such non-static free variables are found,
  then the binding is considered static.

Staticness is a property of expressions which is computed by the
semantic analyser. It's required by the `use` operator.
So curv::Expression has an `is_static_` member, which we can compute
in a mostly bottom up manner.

The minimum requirement is to identify static expressions,
so that the `use` operator can evaluate them at compile time.

As a performance optimization, static expressions are compile time constants,
and are computed exactly once. I want to be careful that the cost of
constant folding doesn't exceed the performance benefit.
* In the case of animation, the entire script is being evaluated perhaps
  hundreds of times.
* In another case, you interactively make changes to a script, evaluate it once,
  then iterate. The entire cost of compilation is currently incurred when you
  press F5. It would be better if compilation took place in the background
  during editing, and if we cached the results of compiling parts of the
  script so that we didn't have to recompile the world from scratch each time.
  One easy fix is to cache the results of compiling a used module.

How is compile time evaluation implemented,
and when does compile time constant folding actually take place?
* I want to replace the Meaning tree evaluator with a byte code compiler
  and evaluator. The latter is faster than the tree evaluator if the same
  subexpressions are evaluated multiple times. Plus, eliminate code duplication.
* So it makes sense to use the byte code evaluator to reduce static
  expressions.
* If I do this strictly bottom up, then 2+3+4 will result in 2+3 being
  compiled to byte code, evaluated to 5 and placed in a Constant node,
  followed by 5+3 being compiled to byte code, evaluated, etc.
  So this could be quite inefficient.
* If I do this strictly top down, then we find that a script file, as a whole,
  is a static expression (even if many of the subexpressions aren't static).
  Top down constant folding just means compiling a script file to byte
  code and evaluating it. This doesn't ensure that static expressions
  are computed exactly once.
* Constant folding is a performance optimization. It has a cost, and isn't
  worth doing unless the expression being folded is evaluated more than once.
* So I need a variation of top down where subexpressions that could be
  evaluated more than once are folded first.

## Modify If Unique
Certain collection operations, like `sort`, are more efficient if they
are done in-place, by modifying an existing collection.

The Modify-If-Unique optimization works by testing if an argument's
`use_count==1`. If so, modify the value in place, if not, copy it first.

It requires a calling convention, "argument is owned by called function",
where the called function is responsible for releasing references on arguments.
This is the natural convention of a stack machine, and the byte code interpreter
will use it.

In an optimized tail call, arguments are moved from the calling function's
environment to the called function's environment. Tail call optimization is
important for modify-if-unique to be effective.

When the last reference to a named value in a stack frame (parameter or
let-bound variable) occurs in a function call argument position, then the
value should be moved, not copied.
* Under C++ move semantics, the vacated slot becomes `null`.
  This interferes with debugging.
* Alternatively, the slot becomes available for reuse for an expression
  temporary or a nested `let`. This lets us minimize the # of slots in
  a stack frame. It's even worse for debugging, unless the debugger is smart
  enough to know the life time of named slots, which requires more
  complex debug metadata. In this case, the debugger will be able to report
  that the named value is not available due to optimization.
* So I guess this optimization should be optional.
* This is related to 'dead variable' reuse.
  It helps reduce memory pressure by releasing values that won't be
  referenced in the future.

## Common Subexpression Elimination
This is usually implemented using SSA intermediate form.
Replace the Meaning tree with an SSA DAG.
Look at Lua-JIT for an implementation.

## Peephole Optimization
Bytecode can be sped up by implementing "super operators" that combine
common combinations of opcodes into a single operation, which eliminates
dispatch overhead within the combination.
It is said that bytecode compilers use peephole optimizers to identify
op clusters and replace them with super ops.

Peephole optimization also refers to strength reduction, which doesn't
have to be done at the compiled opcode level. Lua-JIT does this on SSA form.

Here are some proposed peephole optimizations where Curv numeric expressions
that match a pattern are replaced by calls to primitives in the C library
which are faster and/or more accurate:
* e^x -> exp(x)
* e^x-1 -> expm1(x)
* 10^x -> exp10(x)
* 2^x -> exp2(x)
* 1/sqrt(x) -> invertsqrt(x) // GLSL
* log(x,10) -> log10(x)
* log(x,2) -> log2(x)
* log(1+x) -> log1p(x)
* atan(x/y) -> atan2(x,y)

Peephole optimization is commonly table driven. See Lua-JIT implementation.
