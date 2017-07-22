# Statements

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
