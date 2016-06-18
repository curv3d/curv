# the Semantic Analyzer
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

Phrase::analyze(builtin_namespace) -> Meaning
* start at root_phrase->analyze(builtin_namespace)
* move down the tree, creating meaning nodes and symbol tables. Symbols are
  decorated with types (eg from type assertions) on the way down.
* at the leaves, identifiers and looked up and types become known.
* on the way back up, type information is propagated and the meaning nodes
  are linked together.

The semantics tree could be used to upgrade source files from old, deprecated
syntax to new, supported syntax. This requires that all of the tokens are
present in the semantics tree, with enough of the original structure preserved
that the original source file can be reproduced. It also requires that at
least identifiers have been resolved. So it is an annotated syntax tree.

There are other contexts where we want to convert semantics nodes
to source code.
* message printed by failing `assert`
* printing a function value?
* various debug interfaces?

A possible difficulty is when there are alternate syntaxes with the same
meaning. You'd like these to be represented by the same type of semantics
node.
* Eg, function calls, `f(x)` vs `f x` vs `-x`
* Eg, conditional expressions, `b?c:a` vs `if(b)c else a`.

Possible fixes:
* Suppose that each syntax node has a meaning type that can be predicted
  ahead of time. Eg, `f(x)` is always a function call, `b?c:a` is always
  a conditional. We just have multiple syntaxes for each possible meaning
  (many to one). Then each syntax class can inherit the appropriate
  meaning interface, and semantic analysis modifies the syntax tree by
  filling in meaning fields.
  * I'm not sure identifiers always have the same meaning. Eg,
    constant vs parameter vs global binding?
* The syntax tree is distinct from the meaning tree.
  Each meaning node has a pointer to the corresponding syntax node.
  Very flexible and syntax-independent, at the cost of allocating more nodes.
* In AMI, the syntax tree is made of Phrase nodes, and the meaning tree is
  made of Meaning nodes. But a Meaning doesn't point to a Phrase.
  (Saves me 1 pointer per meaning.) During semantic analysis, the Phrase is
  present, and so is used in constructing error messages. At runtime, I
  apparently don't need access to the Phrase? When compiling an assertion,
  I convert the boolean expression phrase to a string, store it in the meaning.
  I may be relying on one-pass semantic analysis?

Curv should be syntax independent. It's a compiler back end and virtual machine.
The Curv Meaning tree is the API used by front ends.
So, I'll pick the most flexible design.

Okay, but...
To perform semantic analysis, we initially need to traverse a syntax tree
and look up identifiers.
