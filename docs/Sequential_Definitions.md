# Sequential Definitions

Sequential Definitions are an advanced language feature which simulate
the programming style and semantics of an imperative language, but in a way
that doesn't violate the pure functional semantics of Curv.

This feature is complicated and ugly, and arguably should be removed.
However, it exists right now for two reasons:
 1. It's easier to compile sequential definitions into efficient, GPU compatible
    code. Sometimes you need to use sequential definitions, because the current
    compiler isn't yet sophisticated enough to generate the right code from
    the equivalent recursive definitions. This reason will go away once the
    compiler improves, but right now, sequential definitions are
    the only way to render iterative fractals on the GPU.
 2. When you take an existing geometry algorithm written in an imperative
    language and translate it into Curv, it's more convenient to use
    sequential definitions, because it's much closer to the original code.
    This is a possible reason for keeping the feature around.

## Syntax
A sequential definition is just like a normal (recursive) definition,
except that:
* You write `:=` instead of `=`.
* In a statement list (containing definitions and actions), the scope of
  a sequential definition begins at the next statement, while the scope of a
  recursive definition is the entire statement list. This means you can't
  define a recursive function using a sequential definition.
* In a statement list, all of the actions and sequential definitions are
  guaranteed to be executed in the order they are written. This is more for
  the benefit of the compiler than the user. In a pure functional language with
  no side effects, the user normally doesn't care about order of evaluation.

You can't mix sequential and recursive definitions in the same statement list.
The syntax would support this, but the implementation is very complex.

## Limitations
Here's an example of code that works using recursive definitions,
and breaks using sequential definitions, due to the fixed evaluation order
of sequential definitions and the flexible evaluation order of recursive
definitions.

This program aborts due to an "illegal recursive definition".
```
m = {a:=f(0), b:=3};
f x = x + m.b;
m.a
```

This version of the program works fine.
```
m = {a=f(0), b=3};
f x = x + m.b;
m.a
```
