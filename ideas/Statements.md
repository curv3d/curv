# Statement Syntax

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
        echo "f $(x)";
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

## Top level comma phrases

Might as well mean a list, same as if the phrase is parenthesized.

## Top level semicolon phrases

With the `let` proposal, a top level semicolon phrase is a compound action,
which isn't actually a legal program (programs are expressions).

Do I want to reclaim this syntax?
* In "Generalized Scripts", I want action programs, so that `tests/curv.curv`
  has a more convenient syntax.
* In the nodejs REPL, a;b;c means evaluate a and b for their side effects
  and print the value of c.
* In OpenSCAD, a;b;c; will construct a group of shapes and implicitly union
  them. It is "as if" each shape statement has the side effect of drawing
  the shape, which gives a procedural feel.

## Generalized Scripts

Right now, `tests/curv.curv` is a sequence of assert statements (each
terminated by `;`), followed by `{}`, which is an arbitrary return value.
Right now, all Curv scripts are expressions that yield a value.

The `{}` trick is ugly. Maybe a script should be interpreted as either
an expression or a statement, depending on its syntax. This means `file`
is no longer a function, but a metafunction. A call to `file` can yield
an arbitrary Meaning type.

What are the implications? How useful is this? There's not much point in
a script exporting a definition or binder or generator:
export a module/record/list instead.

In an earlier revision of Curv, `a;b;c;` was a block that yields an empty list.
That design would also address my issue.

## `while` loops in list/record comprehensions

Right now, `for` loops are legal in list and record comprehensions,
but not `while` loops.

To fix this, I would need to at least support `a := b` redefinition phrases
(assignment actions) in list/record comprehensions.

To assign a variable, it must first be defined using `var a := b`.
And the definition must be part of the same linear action sequence:
it can't be embedded in a subexpression, because sequential evaluation
isn't defined in the expression world. List and record constructors are
subexpressions.

So, it seems that I will also need to support `var a := b` definitions
in list/record comprehensions. This creates ambiguity in a record constructor:
is a sequential variable definition local (to support while loops) or does
it define a field?

Currently, I use `;` in modules and `,` in records to deal with this ambiguity.
But how about this:
* A module is `module {def1; def2; ...}`.
* A record is `{binder, binder, ...}`
  or `{statement; statement; ...}`.
* A list is `[generator, generator, ...]`
  or `[statement; statement; ...]`.

It seems reasonable for `std.curv` to begin with `module {`.

This would mean `make_shape module {` would show up frequently in `std.curv`,
or I switch to using records.
