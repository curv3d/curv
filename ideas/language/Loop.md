# Loops
Expressive and efficient iteration, without using recursion.

## Fortran
I've designed an embedded imperative sublanguage for OpenSCAD.
It supports imperative programming idioms, including efficient
array update and the quicksort algorithm, without breaking
the referential transparency of function calls.
It's an ugly feature, though, due to the introduction of a large
embedded sublanguage.

## Common Lisp
Richard Waters' "Series" package. Somebody proposed it for OpenSCAD.
It's interesting, but overly complex. It would have to be completely redesigned
to fit into Curv.

## Scheme
I'm considering the named-let feature. Rename it as `loop`,
as in
```
loop id(x1=init1, x2=init2, ...) ...body that calls id(...) recursively...
```

## Sisal
Sisal is "pure functional programming for Fortran programmers".
It provides an interesting syntax for writing iterative array code,
that compiles to efficient code and doesn't depend on recursion.
Maybe some ideas could be borrowed for Curv?

It has two loop constructs.
* Parallel `for` is roughly equivalent to the OpenSCAD `for` loop.
  Iterations can be performed in any order, or in parallel.
* Sequential `for` is roughly equivalent to the OpenSCAD "C-style for".
  It has initialization, test and update sections.

These loop constructs are more complex, but also more flexible: complex loops
with multiple iteration variables are easier, less cumbersome to express than
in OpenSCAD.

On the downside, I don't see any way to implement the quicksort algorithm
(the original, efficient, destructive update version). The Sisal distribution
has a Haskell-style qsort implementation, labelled quicksort.

### `for` loop (parallel)

The 'for' loop is restricted in power so that the iterations can be performed
in any order, or in parallel. So, you can't capture information in one iteration
so that it affects the output of the next iteration.

The OpenSCAD 'for' loop has the same limitation, and I'd say that both
constructs have the same expressive power.

for {range}
  {optional body}
returns {returns clause}
end for

simple {range} is
  i in 1, n         // denotes a range of integers
  a in counters     // counters is an array, a is an element
  x in myarray at i // myarray[i]==x
compound {range} is
  R1 cross R2   // nested loop, basically
  R1 dot R2     // loop over both at same time, terminate when shorter range
                   is exhausted.

{loop body}
Contains definitions of variables that get a new value on each iteration.
Each variable denotes an array within the returns clause.
A variable X can be assigned conditionally, which means you are filtering
the source range.
(Not required, since a definiens expression can be used directly in
the returns clause, without being named.)

{returns clause}
A for expression can return multiple values.
Each return expression is an aggregation clause (constructs an array)
or a reduction clause (typically reduces an array to a single value).

Reduction operators include
  sum <array expression>
  product <array expression>
  ...

### sequential loops: for-initial-while
```
for initial
   i := array_liml(A);
   tmp := A[i];
   running_sum := tmp
while i < array_limh(A) repeat
   i := old i + 1;
   tmp := A[i];
   running_sum := old running_sum + tmp
returns value of running_sum
        array of running_sum
end for
```
I'd say that this construct seems similar in expressive power to
the OpenSCAD proposed 'C-style for' loop.

## Definition Comprehensions
Let's introduce abstractions over definitions. This feature supports
procedural idioms, but in a declarative way, and recasts Sisal loop constructs
into a new syntax.

### 1. Definitions
A simple definition is:
    name = expr

A compound definition is:
    (def1; def2; ...)
This obeys sequential scoping rules. A definition can be replaced by an
echo or assert action. Duplicate definitions are not permitted.

A conditional definition is:
    if (condition) def1 else def2
Def1 and def2 must define the same set of names.

Local definitions:
* I could use `let(...)def`. That would work.
* More convenient is a syntax for marking individual definitions within a
  compound as local. Eg, a `local` or `private` prefix. Which could also be
  used in module scopes to prevent a definition from being exported.
  This becomes more interesting when `local` is applied to a statement that
  bulk-imports names from an external module.
* Or a naming scheme, identifiers with `_` prefix aren't exported.

### 2. Redefinitions
A redefinition defines one or more new variables with the same names as earlier
variables within the same compound definition. It's not the same as assigning
a mutable variable in a procedural language, because when a function captures
a non-local variable, that variable is seen to be immutable.

A simple redefinition:
    new i = i + 1

Redefinitions don't stand alone. They may only be used inside of compound
definitions, and must occur after a definition of the same name.
They redefine a previously defined binding (otherwise a compile-time error).
The `new` keyword is a visual reminder that you are overriding an earlier
definition.

A compound redefinition is:
    (redef1; redef2; ...)

A conditional redefinition:
    if (condition) redef
    if (condition) redef1 else redef2
The then and else branches of the condition don't need to be balanced, and
redefine the same set of names. If a name is missing from one branch, then
it is implicitly redefined to its prior value. Eg, `if(c)new x=x+1`
is equivalent to `if(c)new x=x+1 else new x=x`.

An iterative redefinition:
    while (condition) redef

Eg, this is a compound definition that defines two variables, i and total:
```
(
    i = 0;
    total = 0;
    while (i < len A) (
        new total = total + A.[i];
        new i = i + 1;
    );
)
```

Also: `for` loops: `for (range-specs) redef`

Special redefinition syntax for updating elements of aggregate values:
* `new A.[i] = x` is equivalent to `new A = update_list(A, i, x)`.
* `new R.id = x` is equivalent to `new R = update_record(R, "id", x)`.

### 3. `collect` definitions
This feature mimics a feature of Sisal, and can only occur within the body
of a `while` or `for` loop:

Simple collect definition:
    collect name = generator
The semantics are: once the loop completes, `name` is bound to a list of
all the values generated at each iteration.

Also, compound and conditional collect definitions.

This is low priority, since it will not be familiar to procedural programmers,
and it can be simulated pretty easily using `concat`. In the absence of
an optimizing compiler, there may be a performance benefit.

### 4. Procedures
Functional abstraction over definitions. Procedures are named, and are not
first class values. Procedure parameters are in, out or inout. All output from
a procedure is obtained by redefining out or inout parameters. There are no
global mutable variables, it's not part of the language.

Low priority: you can simulate this using functions and pattern matching
(parallel assignment to extract multiple function results into separate variables).

### 5. Integrating this minilanguage into Curv
* `do <compound-definition> <expr>` evaluates *expr* within the context of
  the names defined by the compound definition. Kind of like `let`, except
  with sequential scoping and definition abstraction.
* `do <compound-definition>` is legal in a script context, or wherever
  recursively scoped definitions are permitted.
  This sounds clumsy, but so far, I don't mind the syntax.
  * This syntax combines recursively- and sequentially-scoped definitions
    in the same scope, in what I think is a semantically clean way.
  * Extending the existing Curv compiler will be interesting.
    We need to pre-compile do clauses to discover what variables they will
    be exporting, before running the Analysis phase.

We could remove the `new` keyword and determine it from context. Would make
the syntax look more "traditional", but maybe it's better to have an explicit
reminder that you are using a pure declarative language.
Eg, `new i = i + 1` is better than `i = i + 1`, it reminds you that you are
defining a new variable `i`.

We can extend `do` notation to permit generators to appear as statements.
Then a `do` statement can be used as a generator (which also defines variables).
Well, this at least makes sense in the module scope, it's more questionable
within list constructors.

do (
    i = 0;
    while (i < len A) (
        translate(i*X) cube(i);
        new i = i + 1;
    );
);

#### Recursive vs Sequential Scope
I want each definition to have a consistent scope. I think this is an
important property for a purely declarative language.

This makes it difficult to fully integrate recursive and sequential bindings.
Do-notation uses strict sequential scoping, so it's impossible to define
recursive functions within that scoping regime. You have to define recursive
functions in a nested child scope, or in a parent scope, where that scope
uses recursive scoping.

OpenSCAD doesn't have consistent scoping, and the behaviour is so disturbing
that it's one of the original motivations for this project.
```
x = 1;
echo(x); // ECHO: 2
x = 2;
```

Python is less fucked up. It is more consistently a sequentially scoped,
sequentially evaluated language. There is the appearance of forward
references within function definitions. The reality is a bit different.
There are only two scopes, global and local. A function definition is
actually an assignment to a mutable variable. References to global variables
are recognized without requiring the global variable to first be "defined".
Python doesn't have the consistent scoping property that I want.

C has consistent scoping. Definition before use, sequential scoping, but a
function definition's scope begins in its own body.

Algol and its immediate descendents have consistent scoping.
At the top level, there are only definitions, there is no expression
evaluation (that is confined to function definitions). So we can have
consistent sequential scoping and sequential evaluation within a function
or procedure, and use recursive scoping at the top level, in such a language.

I came up with a possible solution for the Gen3 project. Forward references
are legal within function bodies. If a function is evaluated before one of
the variables it forward references is initialized, then an error occurs
(run time, or via compile time checking).
* This is similar to Python, so the user base would find it acceptable.
* This is not compatible with redefinitions. A function can't contain a forward
  reference to a variable that is redefined, because it is ambiguous which
  definition is being referred to. (Arbitrarily, we could choose the last.)

Interestingly, forward references from function bodies to variables
are fucked up in OpenSCAD. (On one hand, I can't figure out how to reconcile
sequential scoping with recursive function definitions, and on the other hand,
OpenSCAD implements it, but it's broken.)
    function f() = x;
    a = f();
    x = 1;
    b = f();
    echo(a,b); // ECHO: undef,1

## Sequence Expressions
* In list comprehension syntax, I want a "sequence generator".
  Possible syntax: `(gen1,gen2,...)`.
  It means, evaluator the generators in left-to-right order.
* In definition comprehensions, we have compound definitions.
  Syntax: `(def1;def2;...)`.
  And definitions can probably be replaced by generators. So it's a superset.

So, it seems plausible to merge list and definition comprehensions into
a single concept, "sequence expressions" or whatever.
A 'statement' is either a generator or a sequentially-scoped definition.

`;` denotes sequential evaluation, it sequences two generator expressions,
or two sequentially scoped definitions from definition comprehensions.

`,` denotes a list with no sequential semantics, no defined order of evaluation.
It separates two expressions in a function call argument list or plain
(non-sequential) list constructor.

This unification affects list constructors.
  [expr1, expr2, expr3]
  [stmt1; stmt2; stmt3]

### At the top level of a module:

* Model 0: A module consists of top level definitions, with recursive scope.
  The order of definitions doesn't matter.
  Each top level definition produces 1 or more bindings:
  * eg, a pattern matching definition like `[a,b]=expr`
  * eg, a compound sequential definition (do block), which exports a set
    of distinctly named bindings to the top level. As long as these compound
    sequential definitions don't contain generators, they are order independent.

  Each top level definition is represented by a thunk, which must be evaluated
  the first time one of the bindings exported by the definition is referenced.
  Note that the order in which do blocks are evaluated is data dependent.
  This is because a do block can contain a forward reference to a binding
  exported by a later do block.

* Model 1, Curv: A module consists of top level definitions with recursive scope
  (order independent), plus generator expressions that don't export bindings
  (order dependent).
  
  The generators are eagerly evaluated in left to right order.
  The top level bindings are tied to thunks and are lazily evaluated
  in data dependency order.

* Model 2: Merge 0 and 1: do blocks export bindings and contain generators.
  Now, the order of the do blocks is significant, because the outputs of the
  generators must be assembled in left-to-right order.

  We keep the model 1 design of thunking each do block. The do blocks
  containing generators are eagerly evaluated (in data dependent order,
  due to forward references to other do blocks), and the output of each block
  is stored in a temp slot. Then the results are concatenated in the correct
  order. This means assert and echo statements are not evaluated left to right.
  Echo results are written immediately to the console because it's a debug
  feature, so the output doesn't appear in left-to-right order.

* Model 3: We allow naked sequential definitions at the top level of a module.
  These must be syntactically distinguished from recursive definitions.

  The scope of a recursive definition is the entire module.
  The scope of a sequential definition begins at the token following the
  definition, and extends to the end of the module. So a sequential def'n
  can't contain a forward reference to a later sequential def'n.
  And the body of recursive function definition can't contain a forward
  reference to a later sequential definition.

  Possible syntax:
  rec id = expr;  // recursively scoped definition
  id = expr;      // sequentially scoped definition
  new id = expr;  // sequentially scoped redefinition
  next id = expr; // alt. redefinition syntax
  id := expr;     // alt. redefinition syntax

  Notes:
  * `rec` keyword is found in ML, F#, other functional languages,
    used since `let` bindings are not implicitly recursive.
  * `id = expr;` is OpenSCAD standard syntax for sequential scoping.
    This syntax will make sense to imperative programmers.

  Recursive definitions are thunked, are lazily evaluated in data dependency
  order. An "island" of consecutive sequential definitions can be eagerly
  evaluated left to right, without thunking. However, that entire island
  may need to be guarded by a thunk if recursive definitions are present. Or,
  each definition is separately thunked. An optimizing compiler can replace
  thunking with sequential evaluation, and can merge multiple sequential
  variables to use a single storage location, but only when it is safe
  to do so. If we may need thunking of sequential definitions, we also may need
  to store generator output from sequential islands, since generators may be
  run out-of-order.

  Model 3 introduces the possibility of:
     i = 0;
     rec f(x) = x + i;
     next i = i + 1;
  a recursive definition being interposed between a sequential definition
  of X and a redefinition of the same variable X.
  * possible semantic issues?
  * possible implementation issues?
  * complicates the debug symbol table?

* Model 4: Can we add restrictions to Model 3 so that sequential code can
  be run left-to-right in one pass without thunking?

  Example of problem code:
    i = f();
    i;
    j = 1;
    rec f() = j;
    j;

  (4a) One idea is to use the => operator at the top level.
    local sequential definitions => recursive public definitions
  One issue is that this prevents you from defining public module
  parameters at the top of a script.

  (4b) Storage locations for sequential variables are initialized
  to a special "uninitialized" value. Sequential definitions are evaluated
  in script order. Recursive variables are thunked. If a recursive variable
  tries to read an uninitialized sequential variable, then an error occurs.

### Embedding statements in an expression:
* `let (compound-definition) expr` seems the most obvious.
  You can include assert() and echo(), but not other generators.
* Of course, this means `let(assert(x>0))x` is a way to embed an assert
  in an expression. So I also proposed `do(assert(x>0))...`.
* I considered `(def1;def2;expr)` as an alternative to `let`.
  It looks good, eg `(assert(x>0);x)`.
  However, this "qualified expression" looks too much like a compound
  definition: the former keeps the definitions local, the latter exports them.
* Maybe change the syntax for compound expression.
  The obviously C-like choice is {def1;def2;...}.
  That's even OpenSCAD compatible.
  It conflicts with Curv/OpenSCAD2 record/module syntax, however.
* Maybe `do (def1;def2;...)` for a compound definition.
* Or maybe `{x:1, y:2}` for a record and `module {x=1; y=2;}` for a submodule.
  Also `f(x:1, y:2)` for labeled arguments.
  * Using `:` instead of `=` makes it more obvious these are not definitions
    and no scope is created.
  * The use of `:` is more fluent for labeled arguments.
    You don't mentally insert 'equals' while reading a function call,
    so you can use more English-like labels (ie, not just nouns, but also
    verbs, gerunds and prepositions). See also Swift.
  * The `:` syntax for records more closely resembles Python dictionaries,
    JSON and JavaScript object literals.
  * Need a different token for type declarations. Maybe `::` from Haskell.
* `{}` is ambiguous if braces are used for both record literals and compound
  statements. That's because statements can occur in expression position, eg
  the body of an `if`.
  * Maybe I need 4 kinds of brackets? Let's temporarily use {|...|} for compound
    statements.
  * In argument position of a function call, Curv has f() f[] f{}.
    A compound statement {||} doesn't make sense.
  * In the body of an if, we need [] {} and {||}. () doesn't make sense.
  * Maybe (...) can be overloaded.
    * In an Algol68/Rust style, we have (stmt1;stmt2;expr), which has side
      effects and returns a value. As a special case (stmt1;stmt2;) just has
      side effects.
    * Maybe I get rid of compound definitions (as an expression that denotes
      a definition, a concept not found in other languages). I keep
      compound redefinitions, which play a similar role to side-effecting
      assignment statements in procedural languages, and fit into the same
      syntactic slot.

Here's what I currently like:
* `(def1;def2;expr)` is a qualified expression, replaces `let` expression.
  Eg, `let(a=1,b=2)a+b` -> `(a=1;b=2;a+b)`.
* `assert` and `echo` statements can also occur in a qualified expression.
  Eg, `(a=1;b=2;echo(a,b);a+b)`.
* `[x=1;for(i=a)i+x]` may replace `[let(x=1)for(i=a)i+x]`? Details are unclear:
  can a list constructor contain both `,` and `;`?
* `next i = i + 1;` is a simple redefinition.
  A redefinition can be used in a qualified expression, *after* a definition
  of the same name.
* `(redef1;redef1;...;)` is a compound redefinition. Final `;` required.
* `if(c)redef`, `if(c)redef1 else redef2`, `while(c)redef`, `for(r)redef`
* `f(x:1,y:2)`: labeled arguments in function call
* `{x:1,y:2}`: record literal.

```
sum(a) = (
  i = 0;
  total = 0;
  while (i < len a) (
    next total = total + a.[i];
    next i = i + 1;
  );
  total
);
```

### Within a List Constructor
`[x=1;for(i=a)i+x]` may replace `[let(x=1)for(i=a)i+x]`? Details are unclear:
can a list constructor contain both `,` and `;`?

At the top level, we can have:
* `expression;`, which adds one element to the module.
* `generator;`, which adds 0 or more elements to the module.
* definitions and redefinitions, which may add fields to the module (if not
  local), and which may be referenced by expressions and generators.
* actions (echo and assert), used for debugging, and for constraining the domain
  of definitions.

At the top level, these components are separated by semicolons.

In a list constructor, we have the same components, but all definitions are
effectively local. It's just a sequence of components, there's no semantic
distinction to be made between `,` and `;`.
* I could just permit either character to be used interchangeably.
  Then it becomes a style issue.
  Eg, I can terminate definitions with `;` and separate expressions with `,`
  if I like.

I'd like to introduce a "sequence generator" into List Comprehension syntax.
The obvious syntax is:
    generator ::= gen1,gen2,...
since that's NOT an expression, and since it allows us to define
    expr ::= [ generator ]
as the syntax for list constructors.

How do sequence generators relate to the larger design of sequence expressions?
* In `(def1;def2;expr)`, the semicolons terminate definitions that precede
  the final expression, which computes the value.
* In `(gen1,gen2,...)`, all of the comma separated elements have the same
  role, they all contribute elements to the sequence.

So we have two general syntaxes: list phrases and qualified phrases.
* A 'compound redefinition' should logically be a list phrase.
* A qualified phrase could be represented using an infix operator, like `=>`.
* If the two syntaxes are composed?
  Maybe `(def1; def2; ... => elem1, elem2, ...)`.

In theory, I can add local variables to a loop body using `;`,
but the iteration variables are redefinitions separated by `,`.
The `=>` operator is a better idea.
With `=>`, I can make `,` and `;` synonymous.

`let (a=1,b=2) a+b` is now `(a=1; b=2 => a+b)`.

`[let (a=1,b=2) if (c) for (i=a) x]` is now `[a=1; b=2 => if(c)for(i=a)x]`.

```
sum(a) = (
  i = 0;
  total = 0;
  while (i < len a) (
    next total = total + a.[i];
    next i = i + 1;
  );
  => total
);
```

The => operator is right associative. Is that useful? a => b => c.

How do you embed recursive definitions in a sequence expression?
* `rec` definitions intermixed with sequential definitions at the same level.
  This does involve complex, awkward semantics though.
* The Curv `let` operator supports mutually recursive definitions.
  Embed an island of mutually recursive definitions in a sequence like this:
  s1 => (f(x)=..g(x), g(x)=..f(x)) => s2
  No need for compound definitions to be sequential
  because we have `=>` to express sequencing.
