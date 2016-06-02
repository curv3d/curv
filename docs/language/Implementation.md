== An Idealized Implementation
Based on Javascript and Lua implementations, here's an idealized (but ambitious)
implementation for CurvyScript:
- The lexer produces a stream of tokens.
- The parser converts tokens to a syntax tree.
- Some light semantic analysis is performed on the syntax tree,
  producing an annotated syntax tree (AST). The AST isn't really optimized,
  and remains isomorphic to the original source code. A function value can be
  printed from its AST.
- The code generator converts the AST to byte codes. A register based byte code
  is currently considered best (stack based is simpler, but less performant).
  And byte codes are more popular than threaded code. Ease of conversion to IR
  is a consideration.
- The interpreter evaluates byte codes. During evaluation, it collects
  run-time profile information: types and invocation counts.
- Run time dynamic values are represented using NaN encoding, as 64 bit
  IEEE floats, where all non-float values are encoded as NaN values.
- If a function or loop body is evaluated more than <threshold> times,
  invoke the JIT compiler: possibly in the background, so that the
  compilation takes place in a different core while evaluation proceeds.
- The JIT compiler converts byte codes to an SSA IR (Static Single Assignment
  form, Intermediate Representation), which is then optimized, then compiled
  to machine code. The profile information is used to specialize subexpressions
  for the specific type they are expected to have.
- If an unexpected type is encountered while executing the optimized machine
  code, then fall back to the byte code interpreter.

The overall goal of this kind of implementation is to evaluate a script as
quickly as possible, given that we are always evaluating source code.
We are minimizing compile time + evaluation time.

Lazy compilation: Some subexpressions are stored as ASTs, and not compiled to
byte code until they are first referenced. Eg, top level definitions in a
script or object literal, and wherever else this saves on compile time.

I'd like to have a smart editor, with syntax highlighting and intellisense.
The editor can maintain the AST while you type, and this AST is handed off
to the evaluator.

In the context of CurvyCAD, a strongly-typed subset of CurvyScript will be used
for the functional representation of shapes, and will be compiled to either
GLSL or to optimized machine code using LLVM.

== A Simple Implementation
We'll actually just do a simplified subset of the ambitious implementation.

```
  lexer -tokens-> parser -AST-> analyzer -AST-> generator -VMcode-> evaluator
```
