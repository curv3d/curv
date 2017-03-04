# New Syntax for release 0.0

Goals:
* good enough for release 0.0 (will be revised later after more experience)
* not overly cryptic or clumsy
* powerful enough: not missing essential features

## Language Redesign
Simple, elegant, orthogonal, powerful, looks more like a conventional language.

Lists.
* a[i] is array indexing.
  * a'i is an alternate array indexing syntax.
  * future: a[i..j by k] with i,j,k optional, eg a[1..]
* (a,b,c) is a list. () is empty list, (a,) is singleton.
  * [a,b,c] is an alternate list syntax.
* f(x,y)=... is pattern matching on a list.
* `[i..j step k]` becomes `i..j by k`, is a list, not a generator.
* ...a converts list to sequence generator (ES6 spread operator)

Records.
* {a: 10, f(x): x+1} is a record. No = because no scope.
* f{x,y}=... is pattern matching on a record. Extra fields on right -> error.
  * future: {x,y,...} matches a record with extra fields.
  * future: f{field:param}=... pattern w. distinct parameter and field names

Polymorphic list/record operations:
* s[i] or s'i are list/record indexing with int or string index.
* s[i,j] or s'(i,j) are "swizzling", "slicing". Index argument is a list.
  * r["a","b"] -- this is a list, not a record
* `dom s` is list of indexes of s
* `len s` is # elements of s
* future, `update(index,newval) s`

Function Calls and Chains.
* `f x y z` is a curried function call, like ML/Haskell.
  * precedence of `.`? I guess f(x.i) f(x[i]) f(x'i).
* `f << g << x` is chain notation. Also, `x >> g >> f`.
* in future, ``x `f` y`` means ``f(x,y)``.

Local Definitions and Scripts.
* `def1;def2;phrase` is a block, with recursive scoping:
  * `(def1;def2;body)`: local definitions in a subphrase.
  * top level script: def1;def2;expr -- Value scripts.
* `{def1;def2;}` -- module: recursive scoping, no elements
  * `{}` is the empty record and the empty module.

Use.
* use(file "pathname.curv") -- arg evaluated as const expr in builtin namespace,
  file must evaluate to a module.

## Lists, Records, Array Indexing.
Problem: `a'i` is a non-standard syntax for array indexing.
Better if we used `a[i]`.

### 1. {a,b} is a list
* {a,b,c} is a list. {} is both empty list and empty record.
* polymorphic list/record operations:
  * s[i] or s'i are list/record indexing with int or string index.
  * s[{i,j}] or s'{i,j} are "swizzling", "slicing"
    * r'{"a","b"} -- this is a list, not a record
  * `dom s` is list of indexes of s
  * `len s` is # elements of s
* v[i..j step k] with i, j, step k, all optional. Eg, v[1..].

BUT: what is the pattern matching syntax for lists and records?
The former plan was: {a,b} = r, [x,y] = v

Maybe records are {a: 1, b: 2}. Looks more like JSON, Javascript, Python.
No `=` eliminates confusion about whether there is a scope created.
Instead, `<id>:expr` is a Meaning that adds a binding to a record, and can
be made conditional as in `{if(c)a:b}`.
The : syntax is more fluent for function call keywords, as seen in Swift.

Then, maybe I can use {a:,b:} for record patterns?

Javascript pattern matching:
    [a,b] = [10,20];
    {a,b} = {a:10, b:20};
    {a:foo,b:bar} = {a:10,b:20}; [foo,bar]==[10,20]
    extra fields are ignored

### 2. (a,b) is a list
a[i] is array indexing
(1,2,3) is a list. Apply a function to a list:
* translate(10,0)
* union(square 5,circle 3)
* max(x,y,z)
Pattern matching on lists:
* f(x,y) = ...
* (a,b) = ...

So really a function takes a single argument and returns a single result,
which could be a list. The distinction between max[a,b] and atan2(y,x) is gone.
* This feels like a major simplification.
* Since that distinction is erased, we can now have Haskell-style infix
  operators, eg a `max` b, y `atan2` x, works for both cases.
* An `(x,y,z)` argument list is always a value, can be abstracted.

A list of one element must be written `(a,)`.

OKAY BUT previously, (a,b,c) was a sequence generator phrase.
Use case [ if(cond) (x,y,z) ] has len 0 or 3 depending on cond, as part of
a larger list constructor. I can use 'each' to support this case.
  ( if (cond) each(x,y,z), ... )

OKAY BUT previously, i..j was a generator and [i..j] was a list.
The [] operator performed the conversion. Now what?
* You must write (i..j,) to perform the conversion. Ugh.
* i..j is a list, parens are just for grouping.
  So this means:
  * for (i=1..100) i^2 -- consistent with some other languages
  * (each(1..100), ...) to embed a range in a larger list.

OKAY BUT now a[i..j] == a'(i..j), constructs the list i..j, then uses those list
elements as indexes to construct a slice of a.
* So what about a[1..] as special slice notation? Still possible. `i..` is
  an indefinite range, a Meaning but not an Operation, only valid as a list
  index.
* So a[i,j,k] is seemingly a slice, returns a list of 3 elements.
  Eg, v[Z,Y,X] for vector swizzling.
  Indexing a matrix is `m[i][j]` or `m'i'j`.
  So what about fancy syntax for multi-directional array slices?
  * I don't really need it. Also, is an advanced LA feature.
  * Could use mat[axis1;axis2] in future.

OKAY BUT
    concat vv = [for (v=vv) for (i=v) i];
Previously, the [] operator converted a generator to a list. Now what?
 1. Maybe you need a comma:
      concat vv = (for (v=vv) for (i=v) i,);
 2. Or maybe there is an implicit conversion from generator to expression. So
      concat vv = (for (v=vv) for (i=v) i);
    Which seems okay in the case of a `for` generator.
    It gets murkier with `if` generators.
 3. Or maybe `for` returns a list. Use `each` to convert it to a generator.
    So
      identity(n) = [for(i=[1..n]) [for(j=[1..n]) if(i==j) 1 else 0]];
      transpose(a) = [for(i=[0..len a'0 - 1]) [for(j=[0..len a - 1]) a'j'i]];
    becomes
      identity(n) = for(i=1..n) for(j=1..n) if(i==j) 1 else 0;
      transpose(a) = for(i=dom(a'0)) for(j=dom a) a'j'i;
    Ugh. The []'s are very useful for visualizing the nesting level of the
    resulting list.
 4. Maybe [a,b,c] is an alternate syntax for (a,b,c).
    Can't use it directly as a function argument, since it conflicts with
    array indexing, but it is a better syntax in other contexts.

SO,
* (a,b,c) is a list
* i..j is a list
* [a,b,c] is a list

[(a,b,c)] == [[a,b,c]], by the principle of compositionality.

In (a,b,c), parens are not just being used to group the comma operator.
Instead, both the , and the parens are part of the same grammar production.
Ditto for [a,b,c].

One more context for commas: {a:1, b:2}.

Can commas appear in any other contexts? What about
* top level: a, b, c
* [a=1;b=2;a+b] and [a=1;b=2;a,b]?
* (a=1;b=2;a+b) and (a=1;b=2;a,b)?
No, this is confusing and unnecessary for Curv 0.0.

Unlike comma, which only exists as part of (...), [...] and {...} expessions,
semicolon is an operator with its own semantics, orthogonal to the parenthesis.
It now makes sense for ',' to have lower precedence than ';'.

OKAY BUT I've lost (x,y,z) as a sequence generator. Need an alternate syntax.
Popular names: the "splat" operator (Ruby), the "spread" operator (ES6).
OpenSCAD:   each(x,y,z)
Javascript:  ...(x,y,z)  "the spread operator" (also PHP)
Python:        *(x,y,z)  also Ruby,Perl6
C++             args...  also Go, CoffeeScript
Scheme          args ...
or maybe    with(x,y,z)
            with(args)

Use the ES6 syntax `...expr` and call it the "spread operator".

Note that Javascript spread will also be available inside of object literals.
  aWithDefaults = {x:1, y:2, ...a};
  aWithOverrides = {...a, x:1, y:2};

The args... syntax looks good, when applied to a single identifier.
The Javascript ES6 ...expr syntax (spread operator) is designed to work
in a more general context, eg large list literals.
    union(
        for (i=0..6) each(
            hyperSegment([0,0], q'i, t),
            hyperSegment(q'i, q'(mod(i+1,7)), t),
            hyperSegment(q'i, q'(mod(i+2,7)), t),
        ),
        for(i=0..6) 
            recur_kisrhombille3_7(q'i, q'(mod(i+1,7)), n, t)
    );
    union(
        for (i=0..6) ...[
            hyperSegment([0,0], q'i, t),
            hyperSegment(q'i, q'(mod(i+1,7)), t),
            hyperSegment(q'i, q'(mod(i+2,7)), t),
        ],
        for(i=0..6) 
            recur_kisrhombille3_7(q'i, q'(mod(i+1,7)), n, t)
    );

## Local Definitions

### Goals:

 1. Curv 0.0 has both recursive and non-recursive local scopes,
    the latter because:
    * Non-recursive scopes are easier to compile into GLSL.
    * Use case: locally rebinding a function parameter without changing
      its name, because I want to normalize its value.
    * Sequential assignment can be generalized to support a while loop.

 2. I want recursive `where` bindings for convenience and readability, eg:
      sum v = sums(v, 0) where
        sums(v, total) = if (len v == 0) total else sums(v[1..], total+v[0]);
    Code is written in the same order as an imperative `while` or `for(;;)`:
    initial values of iteration variables, loop condition, next values.

### Analysis
Meeting all these goals creates a complicated design.

 1. Pure Recursive.
    The simplest "pure functional" approach is: all scopes are recursive.
    Like Haskell or Elm.
    * block: (def1; def2; ...; phrase)
    * script: def1; def2; ...; phrase
    Comments:
    * Simple. The order of definitions in a block never matters.
      Helps set the tone that "this is a declarative language".
    * No "sequence expressions" or while loops.

 2. Hybrid Sequential/Recursive.
    Same block syntax, except: definitions by default use sequential scoping.
    The `rec` keyword gives an individual definition recursive scope.
    * More complicated.
    * Can be extended to "sequence expressions" that support imperative
      programming idioms.
    * The `rec` keyword penalizes recursive definitions, makes you think about
      the order of definitions in a module unless you use `rec` everywhere.
    * Has some implementation benefits. Non-rec definitions are easier to
      support in the GL compiler. Rec definitions are more expensive, they
      need a shared non-locals object.
    * Non-rec definitions have a well defined order of evaluation.

Who is my audience? Imperative programmers? Children/artists/mathematicians?

What does most Curv programming look like? High level declarative programs
that glue together library abstractions? Or reproducing imperative algorithms
from the literature on computational geometry?

 3. Use the Pure Recursive design.
    As a future enhancement, consider adding "imperative blocks" which use
    hybrid sequential/recursive scoping.
    * `do (def1; def2; ...; phrase)`
    * `do module {def1; def2; ...;}`
    These are advanced features for imperative-style library programming,
    not for beginners.

### Older Stuff
As a result, this is going to be somewhat complicated. I read an Elm forum
discussion: `where` is requested, but the author rejects that as redundant.
Elm's only local definition form is recursively scoped `let ... in ...`.

A program might use a combination of recursive and sequential scoping.
However, it's not possible to create a hybrid scope where all of the sequential
bindings are visible inside the recursive bindings *and* all of the recursive
bindings are visible inside the sequential bindings. It can't be implemented.
So it has to be one or the other: one of those scopes must be nested in the
other.

Using the Feb2017 let/letrec syntax, nesting different scoping styles is simple:
    let(defs)
    letrec(defs)
    let(defs)
    letrec(defs)
    body

How does this pan out using (;;) sequential scoping and where recursive scoping?
* I'd probably use `where` to redefine self-recursive functions, like `sum`
  above. That style of definition can be embedded in a sequential semicolons
  phrase (aka "prog phrase"). Mutually recursive functions in a prog phrase?
  * f,g visible in def3 and later:
    def1;
    {f,g} = {f,g} where (
        f = ...;
        g = ...;
    );
    def3;
    body
  * (def1;def2;def3;body) where (f=;g=;) // f,g visible everywhere
  * (def1;def2;body where (f=;g=;)) // f,g visible in body

I've considered all of the following:
* letrec(defs) phrase
* rec(defs) -- this is a definition
* phrase where (defs)

### `using`
Here's another idea. Two related syntaxes for local recursive definitions:
* expr with (def1;def2;)
* with (def1;def2;) -- this is a definition. Eg,
      def1;
      with (
        f = ...;
        g = ...;
      );
      def3;
      body

If {def1;def2;} is a module literal with recursive scope, then we could have:
* `using {def1;def2;}` -- a definition
* `expr using {def1;def2;}`
* `using(file "pathname")`

sum v = sums(v, 0) using {
  sums(v, total) = if (len v == 0) total else sums(v[1..], total+v[0]);
};

Now lists, records and modules all use {}.
This conflicts with putting local variable definitions inside a list literal,
as in
    union {
        a=1;
        b=2;
        square(a),
        circle(b)
    }
mimicing the structure of OpenSCAD code.

sum v = sums(v, 0) using module {
  sums(v, total) = if (len v == 0) total else sums(v[1..], total+v[0]);
};

### Hybrid Scope with `rec` Definitions
`(def1;def2;body)` where definitions use sequential scoping,
unless marked with the `rec` keyword, which is needed for recursive functions.

There's only one kind of local definition phrase.

A `rec` definition has the entire LDef phrase as its scope,
but with restrictions:
* The definitions in an LDef have ordinal numbers (0, 1, 2, ...),
  which are an abstraction of time.
* A rec definition remembers the time T of the latest nonrec definition
  that it depends on, either directly or indirectly.
* A nonrec definition at time T cannot reference a rec definition whose
  time dependency is > T. It's a compile time error.

Or we do a runtime check. Initialize nonrec definition slots with missing
and rec definition slots with a thunk, then execute statements in order.
It's a runtime error if a nonrec slot is missing when it is referenced.
"illegal early reference"

This has the feel and smell of imperative programming. It's easy to mix
recursive definitions together with sequential style programming.

Can use the same syntax for top level scripts.
