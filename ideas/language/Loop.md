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
