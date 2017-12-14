# Statement Syntax

## Generalized Actions and Generators
Procedures (Functions returning actions or generators):
* A statement is an action, element generator, or field generator.
  A procedure is a function that abstracts over a statement.
  Procedures are values, but can only be invoked in a statement context.
* Pragmatic justification:
  * It should be possible to replace any use of `for` with tail recursion.
    This requires procedures.
  * It should be possible to use `switch` in any context where `if` is legal.
    Eg, in a list comprehension. This requires procedures.
  * Once I have an interactive debugger, and a rich set of actions for
    controlling the debugger, then procedures are used as debugger scripts.
  * print, error, warning, assert become values.
  * Maybe I get rid of metafunctions, and all of the metafunction bindings
    in the standard prelude become values or keywords. Now the standard
    prelude can be represented as a record value.
* Theoretical justification:
  * You can abstract a statement with a let block.
    According to the principle of equivalence, this means you should also be
    able to abstract these phrases using functions.
  * Suppose I use the Haskell 'monad' trick to implement phrase abstraction.
    Then a statement is a function value (with a weird argument type, of a
    special type of value that can't be counterfeited, like the IO type in
    Haskell). A procedure would then also be a value. This
    demonstrates that procedures can be made theoretically sound.
* Challenges:
  * Can I invoke an arbitrary procedure from the command line?
  * `...` is ambiguous in a weak context. So I can't mark a procedure as being
    an action, element gen or field gen abstraction at value construction time:
    it could be argument dependent, as in `spread x = ... x`.
    So procedure calls are ambiguous in a weak context.
    Does it help if I split `...` into 2 operators?
  * `seq(p1,p2) = (p1();p2())`. Another polymorphic procedure. Since `if`, `for`
    and `;` are polymorphic operations on statements, this naturally leads to
    user defined polymorphic procedures.
  * By creating a set of higher order procedures (combinators), you could write
    procedures whose bodies look like expressions: no syntactic clue that they
    are procedures. A function can be polymorphic across the expression and
    statement worlds. The Haskell Monad argument justifies this as meaningful.
  * Right now, [f(x)] compiles into code that evaluates f(x) as an expression.
    A user can tell by inspection that this is a 1-element list. But no more.

Scripts that denote actions:
* Let's say a script can denote an action. Eg, `print "hello world";`.
  `tests/curv.curv` is simplified. Makes Curv feel more like a procedural
  language, since they all have "action scripts".
* This is parseable. The grammar can already distinguish expressions from
  actions in a weak context. The analyzer could be changed to make this
  distinction statically.
* `file(filename)` is now either an action or expression, depending on filename.
  Is this implementable?
  * `file` is already a metafunction, it can't be a function value since that
    wouldn't be pure.
  * If I change the implementation of `file` so that `filename` must be a
    compile time constant, then the phrase type of `file(filename)` can be
    determined statically and bottom up. There's a security rationale for this
    change: once URLs are supported, we don't want scripts dynamically accessing
    internet servers whose names are chosen at runtime, to prevent exfiltration.
  * If `filename` can be dynamic, then the phrase type of `file(filename)`
    can't be determined statically and bottom up. Which leads to another large
    design decision tree. Which I'll avoid by ruling this out for now.

Scripts that denote generators:
* This is trickier, due to the `...` spread operator being ambiguous in
  a weak context.
* What is the utility? I think it would be terrible style to actually use
  this feature. Just use a list or record expression instead.

What is `x := x + 1`? Can this be the body of a function?
* Ugh. Sometimes the drive towards full expressive power creates monsters.
  Eg, `call/cc`: generalized goto labels as first class values. Leads to code
  that is nigh impossible to understand.
* Don't be misled. From a monad perspective, `x:=x+1` is a pure function that
  transforms a state value to a state value. This state value is some
  abstraction of the scope that variable `x` is defined in.
  * The state value could be a totally locked down, non-counterfeitable
    abstraction of the frame containing the slot for `x`.
    Maybe each frame has a signature, and `x:=x+1` checks that its state
    value is a frame with the correct signature.
  * The state value could be a record containing the field `x`.
    And there's a bunch of research needed to flesh this out and make it
    efficiently compilable.
* The OpenSCAD `do` proposal had procedures with reference (var) parameters.
  This is more actually useful than the ability to mutate non-local variables.
* Maybe mutable variables are deliberately limited and deprecated.
  Use functional style if you want unconstrained abstraction.
* A procedure cannot reassign a nonlocal sequential variable.
  Use a function instead: `(V1, V2) := F(V1,V2,0);`

I think this is technically achievable.
But it's a can of worms with limited utility.
Consider defining a subset of this that provides the greatest benefit,
to limit the implementation and design cost.
Not for MVP.

## Semicolon Phrases

Semicolon phrases work everywhere that it makes sense:
* (a;b) -- simple grouping of actions, generators, binders, and definitions
* [g1;g2] -- list
* {b1;b2} -- record
* {rd1;rd2} -- recursive module
* {sd1;sd2} -- sequential module

When you type an unadorned semicolon phrase into the CLI:
* a1;a2 is a compound action, we execute the actions.
* rd1;rd2 is a compound recursive definition: we define all the things,
  and this is how you define mutually recursive functions in the CLI.
* sd1;sd2 is a compound sequential definition.
But we don't support naked generators or binders in the CLI.

## Generalized Definitions
* id = expr
* def1; def2; def3 -- can also contain actions
* ( definition )
* `[x,y,z] = expr` and other patterns
* `use {a=b;...}`, `use (constant in std namespace)`
* `if (cond) (a=foo;b=bar) else (a=bar;b=foo)` -- low priority

We attempt to recognize a generalized definition in these contexts,
which create scopes:
* bindings argument of `let` and `do`.
* top level phrase in a CLI command. If not a definition, analyze as an
  operation.
* argument of `{}`. If not a definition, analyze as an action/binder sequence.

## `do` syntax

Proposal:
* `let <recursive-definitions&actions> in <body>`
* `do <sequential-definitions&actions> in <body>`
* `{ <recursive-definitions&actions> }` -- module syntax

Maybe?
* `<body> where (recursive-definitions&actions)`
* `<body> where recursive-definitions&actions` -- better for scripts
* `<body> do_where (sequential-definitions&actions)`
These are chainable, just as `let` and `do` are chainable.

Benefits:
* `do print msg in ...` reads better than `let print msg in ...`.
* Sequential definitions and `while` actions can only occur within `do`,
  so we can refer to the sequential sublanguage as the `do` feature.
* Ideally, I'd like to not mix recursive definitions and actions.
  I'd like to cleanly separate functional and imperative code.
  With this syntax, I can at least avoid actions in `let`.
  But, I think I need to support asserts and debug actions in modules.
* The `Statement_Analyzer` might be easier to write if there are
  separate recursive and sequential versions.

Sequential definitions can't be used directly in a module.
That's okay; you typically need local iteration variables to build
something using a `while` that you wouldn't want to export.
So it's not that great a feature anyway.

## Separate Actions from Recursive Definitions

In a recursive definition list, separate the actions from the definitions.
Move the actions after the definitions, don't intermix them.
* Contexts are: let defs in body, body where defs, {defs}
* But note:
  * `let defs in do actions in body` is already available for `let`.
  * `do actions in body where defs` is available for `where`.
  * For a module, {defs; use {actions}} is possible, assuming sequential
    modules work. This lets you put actions anywhere a definition is valid.
* Potential useability benefits.
  * The relative order of actions and recursive definitions in an intermixed
    sequence doesn't matter. Maybe we confuse the user by requiring them to
    choose a relative ordering, as if it affects order of evaluation, the
    way it does in a `do` block.
  * Simpler documentation: the order of definitions doesn't matter/the order
    of statements doesn't matter.
  * In a polymorphic context (recursive/sequential definition list),
    it's easier to recognize a recursive definition list (first statement
    is an `=` definition.
  * In a large multipage module, it's easier to find the actions
    and review the ordering of side effects.
* simpler implementation.
  * In a semicolon phrase, easier to distinguish recursive/sequential stmt list.
  * ...and simpler data structure.
* possible syntaxes:
  1. In a recursive definition list, definitions precede actions, and first
     statement is a recursive definition.
  2. Introduce a keyword separating recursive definitions from actions.
     Eg, 'actions' or 'perform'.
     {
       a=1;
       b=2;
     actions
       print "my module";
       assert(a+b<10);
     }
* drawbacks
  * In a large multipage module, assertions should be written adjacent to the
    definitions that they guard, for locality of code.
    Although you could use a nested compound definition,
      (
        def1 = whatever;
        def2 = something else;
      perform
        assert(def1 0 == def2 1);
      );
  * More syntax to learn; doesn't make the language more powerful.

## Semicolon Phrases are Generalized Compound Phrases

Proposal:
A semicolon phrase isn't a block; it is a generalized compound phrase.
A semicolon phrase may contain actions, plus at most one of
definitions, generators, or binders. This creates a compound action,
definition, generator, or binder.
* Definitions in a semicolon phrase *do not* create a local scope.
  You must use `let` or `do` to create a local scope.
* The meaning of a semicolon phrase can be determined bottom up.
  Parenthesizing a semicolon phrase doesn't change its meaning.

This supports our current need for compound actions, compound generators
and compound binders. It is compatible with proposed future syntax:
while loops in list/record constructors, and conditional compound definitions.

`1;2;3` is a compound generator, equivalent to `...[1,2,3]`.
`[1;2;3]` is now a list constructor.
`if (c) (a;b;c)` is a conditional compound phrase.

## `while` loops in list/record constructors

This is low priority.

Right now, `for` loops are legal in list and record constructors, but not
`while` loops.  To fix this, we need to extend list/record constructors with:
* `while` loops containing generators/binders
* `a := b` reassignment actions, with a matching sequential definition in scope.

Proposal:

 1. The `while` action is generalized so that the body can contain generators
    or binders, same as how the `for` operation is already generalized.

 2. If the body of a `do` is a record or list constructor, then the `do`'s
    sequential scope is continued into the body of the constructor.
    Reassignments and `while` phrases may be used in the constructor body.

    `do <sequential definitions> in [ <generators, actions, reassignments> ]`
    `do <sequential definitions> in { <binders, actions, reassignments> }`

    This syntax segregates the definitions (which are outside the constructor)
    from the generators/binders (which are inside the constructor).
    It avoids the ambiguity of putting local definitions (which don't define
    a field) inside a record constructor.

`while (c) (a;b;c)` is an iterative compound phrase.

`do var i:= 0 in [while (i < 10) (i; i:=i+1)]`

## `let` syntax

Proposal:
The Algol-68 block syntax is replaced by `let def1;def2;... in body`.

Benefits:
 1. The difference between a block and a compound statement is now
    bigger than a single semicolon.
    The compound statement (a:=1;b:=2;) doesn't turn into an illegal block
    with body 'b:=2' if you leave out the last semicolon.
 2. Newbies are less likely to confuse `x=1` with an assignment statement
    in `let x=1 in f x` vs `(x=1; f x)`.
 3. The scoping of `x=1` vs `x:=1` should be more obvious now that you
    need `let` to introduce a new scope. Parentheses can't introduce
    a new scope by themselves.
 4. Can chain together recursive and sequential lets without parens piling
    up at the end. Removes the motivation for hybrid scopes that combine
    recursive and sequential definitions.

Local definitions in a shape script:
  ```
  let
  name1=value1;
  name2=value2;
  ...

  in
  shape
  ```
I can search for '^in' to find the shape expression.

In a function definition:
```
noise xy =
  let
    var i := floor xy;
    var f := xy - i;

    // Four corners in 2D of a tile
    var a := random(i);
    var b := random(i + (1, 0));
    var c := random(i + (0, 1));
    var d := random(i + (1, 1));

    var u := f * f * (3 - 2 * f);

  in
    mix(a, b, u'X) +
        (c - a)* u'Y * (1 - u'X) +
        (d - b) * u'X * u'Y;
```

This `let` syntax eliminates the usual reason for function bodies,
then and else clauses, to be parenthesized in functional code.
It eliminates editing the end of a function definition when local variables
are added to the beginning.
```
    foo =
        some large
        multi-line expression;
```
Now add some local variables:
```
    foo =
      let
        a = 1;
        b = 2;
      in
        some large
        multi-line expression;
```
I don't need to add a `)` before the final semicolon. I don't need to edit
the final line.
```
    foo =
      let
        a = 1;
        b = 2;
      in let
        var x := 0;
        while (...) (
          ...
          x := x + 1;
        );
      in
        some large
        multi-line expression;
```
I can mix recursive and sequential scopes by simple chaining,
without `)`s piling up at the end.
There's no need to introduce mixed recursive/sequential scopes,
you just use chaining.

This also implies using `let` to introduce debug actions. Eg,
```
    f x = let
        print "f $(x)";
    in
        ...;
```

## `where` syntax
In addition to `let`, also support `body where (def1;def2;...)`.

At top level,
  ```
  shape

  where
  name1=value1;
  name2=value2;
  ...
  ```

As per above, maybe `where` consumes the rest of a list?
In Haskell, `where` is the most popular way to define local bindings
in a function definition. A greedy `where` would require parentheses
around the function body, so
    ```
    noise xy = (
        mix(a, b, u'X) +  
            (c - a)* u'Y * (1 - u'X) +  
            (d - b) * u'X * u'Y 
    where
        i = floor xy;
        f = xy - i;

        // Four corners in 2D of a tile
        a = random(i);
        b = random(i + (1, 0));
        c = random(i + (0, 1));
        d = random(i + (1, 1));

        u = f * f * (3 - 2 * f);
    );
    ```

Another syntax:
  ```
  let (bindings) (body)
  let (bindings) << body
  body >> let (bindings)
  ```

## Top level semicolon phrases

With the `let` proposal, a top level semicolon phrase is a compound action,
which isn't actually a legal program (programs are expressions).

Do I want to reclaim/legalize this syntax?
* In "Generalized Actions", I want action programs, so that `tests/curv.curv`
  has a more convenient syntax.
* OpenSCAD2 geometric objects.

## Static Phrase Kinds

Top-down static phrase kind determination during analysis.
Low priority (not MVP).

Rationale:
* Semicolon phrases can combine any kind of phrase.
* Can I type a semicolon phrase in the CLI and test what it does?
  Initially, I wanted the CLI to execute any phrase kind (so, a binder is
  executed by printing each field id:val, one per line). This might require
  static phrase kinds, so we know what Operation virtual function to call.
  Or it requires a new Operation virtual, `polyexec`.
* Static phrase kinds also seem appropriate for compiling Curv code into
  Instructions. (But not absolutely necessary.)
* Outside of the context of [] or {}, we don't statically know if `...expr`
  is a generator or binder. This means:
  1. If we want a bottom up static phrase kind for `...expr`, then I guess we
     need different syntax for the spread operator in [] and {}.
  2. If we want "dynamic phrase kind determination" for spread, then we need
     Operation::polyexec.
  3. Or we have a top-down static phrase kind for `...expr`, relying on
     the context of [] or {} to determine the kind. If `...expr` appears
     in a weak context, it is a compile time error. This seems reasonable.

I'll take #3.
* Top-down static phrase kinds.
* The CLI supports expressions, definitions, and actions (for debugger control).
  But not generators, binders or metafunctions: these are syntax fragments,
  not stand-alone phrases.
