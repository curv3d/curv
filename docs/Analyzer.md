== the Semantic Analyzer
It converts a parse tree to a semantics tree.

If I am targetting LLVM (currently, yes), then the semantics tree is
what I need to generate LLVM code.
* Look up identifiers.
* Figure out dependencies between definitions in an object literal or let.
  Identify groups of mutually recursive definitions.
  Identify function calls which may be recursive.
  Serialize non-recursive definitions & recursion groups into dependency order.
* Identify tail calls.
  Transform tail calls into loops.
* Type inference, so that I can generate faster code when types like 'double'
  are known at compile time.
* Constant folding, needed since types are represented by predicate values.

Hm, maybe my treatment of tail recursion will allow semantics trees to be
directly interpreted, with tail call optimization. I didn't previously think
this would be feasible.
