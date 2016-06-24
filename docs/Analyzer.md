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
* The Meaning tree doesn't currently preserve all of the source.
  It only preserves enough source to reconstruct the Location of an expression
  for error reporting.

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

## Meaning Notes
How do I represent operations and functions? Eg, the binary + operation
as in 'x+y'?
 1. I could create a distinct subclass of Expression for each built-in operator.
    Eg, Binary_Plus_Expression.
    * So how is sin(x) represented, and do I use different representations
      based on syntax? One difference is that 'sin' is a builtin in TeaCAD,
      so I need a 'sin' Value and 'sin' Bindable anyway, whereas I've made no
      commitment that there is a '+' function in the builtin namespace.
 2. General purpose function application node, with a function and a
    positional argument list. (No keyword arguments yet.) Works for x+y, sin(x),
    etc (code reuse).
    * How far does this go? x&&y? if(a)b else c? for(i=s)x? [x,y,z]?
      Are all of these compiled into application nodes? (Well they would be
      in Lisp.) Eg, __for__(i,[1..10],i+1).
    * With this design, the Call_Expression has a function Expression,
      which has a source Phrase. What do I use for this source Phrase?
I think that a reasonable and non-surprising approach is to use special
meaning nodes for magic syntax, and function call nodes for function
call syntax.

Does the semantic analyzer use multiple passes? Is different information stored
in the meaning tree depending on the pass? AMI had a single bottom up semantic
analyzer (including type checking). That will currently work here, for now.
But Curv may evolve to be more complicated.
 1. bottom up type checking is fine for now.
 2. identifier resolution: we support unrestricted forward references in
    scripts.
     * First pass: parse the script. Each top level entry is a definition,
       or an 'action': expression, for loop, echo, assert, etc.
       Put the definitions into a Namespace mapping name to Phrase.
       Put the action phrases into an ordered list.
     * Second pass: during evaluation, when a name is looked up,
       JIT compile the phrase into an Expression, stuff the Expression
       back into the namespace, then evaluate the expression.

How to evaluate a script:
 1. Parse the script into a Script_Phrase object.
    This contains: a namespace mapping a name to a Phrase or Expression,
    and a list of action phrases. Due to JIT, we don't have a clear
    distinction phase distinction: a Script is both a Phrase and a Meaning.
    Duplicate definition is a parse time error.
 2. Evaluate the script by evaluating each action, with the following effects:
     * partially JIT compile the Script
     * produce a list of values
     * cause side effects (echo and assert)
    An action has a side effect: it 
How to evaluate a command line:
 1. Parse it into a definition or action (including echo/assert/for).
 2. Evaluate the phrase. Action is evaluated immediately: each time a list
    value is generated, it is printed to console. 'generate_item' is virtual?
    Definition means update the console Namespace.
     * How are references resolved?
        * References are relative to the console namespace before it is updated.
          No recursive definitions. Existing definitions don't change their
          behaviour when an existing definition is overridden.
          Compile the definiens *before* updating the console namespace.
          x=x+1 increments x, an anomaly.
        * References are relative to the new namespace. Recursive defs allowed.
          Existing definitions must be recompiled if one of their dependencies
          is overridden. Harder to implement, and, it's not that important
          right now.
       Either way, compile the definiens now and show compile errors.
       (But, this may prevent mutual recursion in #2: reference to a definition
       that hasn't been entered yet. Or maybe that's not supported?
       In #2, maybe undefined reference is just a warning? Too hard for now.)

Maybe a Script exists in all 3 phases: parse, analyze and run time?
It has a value list, which may be just parsed (only a list of action phrases)
or evaluated (list of values available). It has a namespace, which may contain
parsed definiens and compiled definiens.

But, a Script may have non-local references to parameters. So there are
multiple distinct values generated from a single script phrase.
So, a Script_Value (aka Object) contains a list of Values, and a reference
to a namespace, which may include unanalyzed phrases?

But, it's more complicated. A definiens may be simple: not a function literal,
not a thunk. In that case, it makes sense to fully evalute it to a value,
taking into account external parameters, and storing the result in the Object.
 * Fine, but let's make a distinction between top level scripts that don't
   have external parameters and are partially JIT compiled, vs internal
   scripts that are compiled all-or-nothing. When an internal script is
   compiled, we determine the number of value slots it needs for its
   instances.

A Top_Level_Script exists across all 3 phases: parse, analyze and run.
 * It's a Ref_Value. It's a Meaning (or encapsulated as a Constant).
   It's a phrase (but maybe not a curv::Phrase, as it doesn't appear as an
   interior node of a parse tree).
It has a value list, which may be unanalyzed (only a list of action phrases)
or evaluated (list of values available). It has a namespace, which may contain
both unanalyzed and compiled definiens.

An internal script is either unanalyzed or fully compiled, all or nothing,
existing in either the parse or the analyze phases.
It may have multiple instances at runtime with different contents, depending
on (eg) the values of external function parameters.
