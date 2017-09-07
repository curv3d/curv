# New Syntax 3 (Sept 2017)

## array indexing a[i]

The ' syntax makes more sense. a'i is regular indexing (i is an integer),
while a'[i,j] is the vectorized form.

a[i] is regular indexing.
a[i,j] is a shortcut for vectorized indexing, equivalent to a[[i,j]].
I find the latter confusing (the inner and outer brackets have different
meanings, and the shortcut equivalence is not obvious).

a[i..j] would be a nice shortcut for slice notation, currently unimplemented.
Although you can use a[[i..j]] or a'[i..j].

a[i,j] would also be a reasonable shortcut for a[i][j] / a'i'j.
It could be generalized to support multi-dimensional array slicing.
Which I might need in the future.

## generalized scripts
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

I could make record and module syntax more distinct. Eg, use `;` in modules
and `,` in records.

## Semicolons
I hate that definitions end with ',' in std.curv but end in ';' in a top level
block. Copying definitions from one context to another requires syntax change.

So allow ';' in both contexts. { def1; def2; ... }

Right now, ';' creates a block everywhere, even in braces.
{x=1;echo x} is legal, yields {} plus an echo effect.
(comma binds looser than semicolon--the opposite of English)

With this proposed syntax change, I think that ',' and ';' no longer have
a precedence relation.
  program ::= list EOF
  list ::= commas | semicolons
  item ::= ...
Mixing commas and semicolons at the same level will be a syntax error.

The meaning of semicolon is now context dependent. Forms a block at top level
or in parentheses. In braces, you get a module. In brackets, probably illegal.

## Mixed Modules/Records
For defining shapes, I have considered code where the define_shape argument
contains some definitions, then a `...` clause that inherits fields from another
structure. Eg,
    cube = make_shape {
        inradius r = make_shape { <cube constructor> };
        circumradius r = make_shape { <cube constructor> };
        ... inradius 1
    }

The recursive `use` operator is hard to implement.
So, I thought, introduce `...` as a field generator, allow field generators
and definitions to be combined in a structure literal. That's easier because
the bindings brought in by `...` don't have to be known during analysis.

If I allow definitions and field generators to be mixed in the same structure
literal, then what happens if there is a name conflict?
* run time error
* override. The field generator overrides the definition.
  Field generators are supposed to override each other, left to right, so that
  defaults and overrides can be programmed, like in javascript.
  If an fgen overrides a definition then the fgens should be written after the
  definitions, for clarity. Maybe that's enforced.

## Mixed Recursive/Sequential Definitions
If we can mix definitions and field generators,
can we mix recursive and sequential definitions?
Yes, as previously designed.

## Statements
There are a number of contexts where you can include actions as statements:
* a block
* a list constructor
* a structure constructor

The latter 2 contexts allow actions as statements, but not local bindings.

Currently, a list constructor can use `for` as a generator, but not `while`.
To fix this, permit local bindings in a list constructor.
What's the syntax?
* Currently, (a,b,c) is a list, (a;b;c) is a block.
  Now definitions are permitted in both contexts.
