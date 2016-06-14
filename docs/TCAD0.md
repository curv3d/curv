# TCAD 0.0 milestone: "Desk Calculator"

An interactive desk calculator.
No script file parsing, user-defined functions or {...} literals.

* string literals, [a,b,c] list literals, no list comprehensions
* seq@i, len(seq), concat[s1,s2,s3]
* && || !, if-else, relational ops
* Built-in functions only. No optional, keyword args.
* f(x,y) only, no right associative function calls.
* Interactive 'id=expr' definitions only, expr is evaluated in previous env.
*  No 'let', no {...}, no local definitions.

Focus on data structures.
Use a 'Meaning' tree interpreter.

## Boolean
* && || !, if-else, relational ops

## Number
print floating point numbers accurately.

sum, product, max, min
mod(x,y), abs, floor, round, ceil
trig: pi, tau, deg, sin, cos, tan, acos, asin, atan
exp: e, x^y, log(x,b=e), sqrt(x)

A parsimonious numeric API:
Instead of providing a large number of 'special case' numeric functions
like expm1(x) == e^x-1, you instead write 'natural' code and the compiler
will detect common patterns and rewrite them into more efficient/accurate
code. For example:
* e^x -> exp(x)
* e^x-1 -> expm1(x)
* 1/sqrt(x) -> invertsqrt(x) // GLSL
* log(x,10) -> log10(x)
* log(x,2) -> log2(x)
* log(1+x) -> log1p(x)
* atan(x/y) -> atan2(x,y)

## Sequence
* seq@i, len(seq), concat[s1,s2,s3]
* concat[] returns [], not ""

## String
literals: "abc", but no escape sequences.

`struct String:public Ref_Value {size_t n; char data[n];}`

`mk_str(const char*)`

## List
literals: [a,b,c]

`struct List:public Ref_Value {size_t n; Value data[n];}`

`mk_list(size_t n)`

## Identifier
An Identifier phrase is resolved to a builtin.

The builtin namespace is a map from name to Constant.

curv::Expression::eval() -> Value

curv::Phrase::analyze_expression(ctx) -> Expression
Analysis_Context is builtins
::lookup(id) -> Expression

## Function
only builtin functions

Builtin:
* parse time: unresolved Identifier
* analysis time: Constant. Later we'll need more info for optimization,
  but it should still be a Constant node.
* run time: Function object with a virtual apply() method that
  maps a sized array of Values to a Value.

Function call: f(x,y)
* parse time: Call_Phrase(fun-phrase, list(arg-phrase))
* analysis time: Boxed_Call(meaning,list(arg-meaning)).
