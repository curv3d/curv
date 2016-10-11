# Implementation of Functions and Modules

## TODO: Recursive Functions
* Script-Module recursive bindings
  * Lambda_Phrase has data field 'bool recursive'. Default false, set true if
    definiens of a module binding. It means: During analysis, shared nonlocals
    dictionary is constructed by module, so don't use own nonlocals dictionary
    and slot mappings when resolving nonlocals, delegate to parent module.
  * Lambda_Expr::eval won't be called. 
  * Bindings_Analyzer contains the shared nonlocals dictionary. Right now,
    there are no private bindings, and for script modules, all nonlocals are
    builtin constants, so the current dictionary doesn't need any more entries.
  * Bindings_Analyzer::add_definition() should set lambda->recursive = true.
    Do we need to perform additional book keeping to keep track of the
    lambda entries, like a flag in the dictionary entry?
  * Module_Environ::single_lookup must now return a Module_Ref or a
    Nonlocal_Function_Ref, based on the dictionary lookup. Either store a
    lambda flag in dictionary (need an analysis-time Dictionary type), or use
    an isa test on the slot_phrases vector
    (Module_Environ must reference the Bindings_Analyzer).
  * Bindings_Analyzer::analyze_values() must store a constant, lambda or
    thunk. I can determine this using current data structure.
* `Bindings_Analyzer` (or just `Bindings`). This seems like the key abstraction.
  Will evolve the data structure and API to support additional planned features.
  * supported language features:
    * scriptmodule. each slot <=> a module binding. dictionary contains
      public module bindings only.
    * submodule. each slot <=> a module binding or a nonlocal. All slots
      nonlocal. internal dictionary of module bindings and nonlocals.
      public dictionary (for Module value) of module bindings only.
    * let expression. local slots: let bindings. nonlocal slots: let bindings
      and nonlocal bindings. The same let binding can be in two places (to
      avoid this, we need an extra compile pass).
      * How to determine which slot to use for a given lookup? Easy, change the
        env passed to analyze() based on whether we are analyzing a function
        definiens.
      * Could have 2 dictionaries, local and nonlocal. Or, a unified dictionary
        plus a bool in the env used during lookup.
    * pattern matching definitions. non-trivial definiendum patterns result
      in a local temp slot (for the value to be pattern matched), plus local
      or nonlocal let/module slots for the bound names.
    * parameterized module. Here's one simple implementation:
      * to call a module, copy the nonlocals list, then update parameter slots
        in that list using argument values.
      * a definiens that references parameter slots is compiled into a thunk
        that is evaluated every time the binding is referenced.
  * the data structure should keep track of:
    * nonlocal slots (environment vector for module or recursive functions
      in a let)
    * argument slots
    * local slots (including anonymous temporaries)
    * mapping from names to slots
  * For Sub-Modules, `Bindings` needs:
    * Dictionary from field names to slots, for Module. (no nonlocals).
    * List of compile time slot values, for Module (fields and nonlocals).
      Construct in phase 3, after analysis of definientia.
    * Vector of slot exprs, constructed during phase 2 (analysis of definientia)
    * Vector of slot definientia phrases, phase 1.
    * Dictionary from names to slots, constructed phase 1 & 2.
      Used for Environ lookups in phase 2. Also need flag (is this a function
      member?), which: could be in dictionary, could be determined from slot id
      using: is_function_member(slot) = is this a member slot (range check)
      and is definiens a Lambda_Phrase (from vector of definientia phrases)?
  * For Script-Modules only, we don't remember nonlocals in phase 2, so:
    * `dictionary`: Dictionary from field names to slots.
      Used for Module value, and for Environ lookups in phase 2.
  * Eh:
    We add a sequence of definitions (name/phrase pairs).
    Then we begin analyzing phrases, we discover nonlocals, and add them to
    the dictionary and slot map as name/expr pairs.
    In the end, we'll produce a map from slots to exprs, which we convert
    to a List of compile time values.

* Recursive functions
  * How do I hook analyze_lambda() so that it uses a shared nonlocals dictionary
    for module function members?
    * Modify the Lambda_Phrase so that it knows it's a module member.
      Add a new data member (bool or pointer).
    * The shared nonlocals dictionary is in the env passed to analyze_lambda.
      * So analyze_lambda checks the bool, downcasts the env to access the
        dictionary?
      * Or the new Lambda_Phrase data member is a pointer that provides access
        to the dictionary? (Does the referenced object have the same lifetime
        as the Lambda_Phrase?) This reminds me of a proposal to add an Environ*
        to every Expression so that the debugger can look up symbols when a
        program is suspended.
      * Or, the lookup() function has a second flag argument that defaults to
        false and is not propagated to parent lookups. If true, the lookup is
        cached in the shared nonlocals dictionary.
    * When a nonlocal is looked up, a function in the same scope as the
      recursive lambda is treated specially (a Function_Ref rather than a Ref).
      How is that triggered?
      * Normally analyze_lambda() calls lookup() on its env argument. But now,
        we need to know the lookup is coming from a recursive function.
        So, an extra flag argument to lookup() that defaults to false and is not
        propagated to parent lookups. If true, then:
        * Cache the result in the shared nonlocals dictionary.
        * If the result is a function in this scope, return a Function_Ref.
  * Run time data structures:
    * Lambda is an improper value type, a lambda expression without the data
      for nonlocal values.
    * Closure is a proper value type, a Lambda + nonlocal data, the latter
      represented as a List.
    * a Module's slot array contains field values and nonlocal bindings
      referenced by function fields, represented as a List.
      * For function fields, the slot contains a Lambda not a Closure.
      * For nonlocal function bindings, the slot contains a Closure.
        At first glance, that could lead to reference cycles, as in:
          m = {f(x) = g(x);};
          g = m.f;
        Except that this generates an "illegal recursive reference" error.
    * In a module frame, slot 0 is the Module's slot array (a List value).
      Alternatively, I could change Frame to use `List& nonlocal`, then I don't
      need slot 0. Same performance.
    * A let expression has a frame slot containing the nonlocal values
      referenced by function bindings. Slots for function bindings contain
      Lambdas.
  * Analysis time data structures:
    * a Local_Function_Ref contains a lambda_slot and a nonlocals_slot.
      The eval() function combines these into a Closure value.
    * A Module_Function_Ref contains a lambda_slot (nonlocal) and implicitly
      references local slot 0, combining these into a Closure value.
  * Special cases:
    * Recursive static functions (non closures), C function equivalency.
    * supported: Self-recursive function.
    * supported: Mutually recursive functions in the same let or module scope.
    * causes an error: any other kind of recursive definition.
  * RFunctions: All of the functions in a mutual-recursion group (within a
    let or module scope) share the same nonlocal slot vector. These functions
    reference themselves and each other, not by Constant nodes, but by
    nonlocal references, and the slots themselves contain RFunction refs,
    not proper Values. The shared slot vector is represented as a List and
    stored in an additional slot associated with the let or module expression.
    * Hence, recursive functions are not Constants.
    * An RFun reference is a special Meaning class that contains the slot ids
      of the RFun code reference and the nonlocals list. There are local and
      nonlocal variants. When eval'ed, an RFun reference fetches the contents
      of the two slots and constructs a closure Value.
    * Future optimization: use a special operation to call a function via an
      RFun ref without constructing the closure as a temporary Value.
      (Peephole optimization: super-operator.)
  * How to analyze a recursive function.
    * Analysis of a phrase tree is a depth-first recursive process.
      Previously, I map a let/module bound identifier to a slot reference
      with no further analysis. Now, I recursively analyze the definiens phrase,
      and store the resulting Expression.
      * If it is already analyzed, then it's a thunk or rfun.
        Return the corresponding ref type.
        Future optimization: add a third category, proper value.
      * If is an unanalyzed phrase, then mark the symbol table entry as pending,
        then analyze the phrase.
      * If it is a pending phrase, then if a lambda phrase, mark it as an RFun,
        otherwise mark it as a Thunk. analyze() returns a slot reference.
      * Future optimization: add a fourth category, Constant.
    * RFuns in the same mutual recursion group must share a nonlocals list,
      otherwise, if they have separate lists, then there will be reference
      cycles between these lists.
    * Therefore, after analyzing all of the definitions in a let/module scope,
      visit all of the RFun bindings, amalgamate all of the nonlocal arrays
      into a single shared nonlocal array for the scope. A new compile phase,
      post analysis, is needed to assign nonlocal slot ids.
    * A nonlocal ref created during analysis needs to store a key that can be
      mapped to a slot id after amalgamation. This is a Ref_ID, possibly a
      bitfield containing a stack depth and slot index. Or an Atom.
    * As a special case, the nonlocal array for a module will use the same slot
      array as the field values.
  * Simplified Design: All let/module function definitions are analyzed as
    RFunctions. Nonlocal slot indexes are assigned during analysis, no need for
    a new compiler pass. If a let has function definitions, then an extra slot
    is added for the function nonlocals list. The slot contains a thunk for
    a list constructor, so it's lazy. In a module, the nonlocals are added
    directly to the module's slot array.
* lambda expressions: x->expr: stack of frames
  * lambda syntax: a->expr, (a,b)->expr, [a,b]->expr, {a,b}->expr
    * Currying: (a)(b)(c)->expr, like a->b->c->expr.
      This abbreviation hardly seems worthwhile, only shows value in
      the context `f(a)(b)=...` definition which is patterned after ML.
    * Chains: `f x->x+1` parses as `f(x->x+1)`.
      Not a high priority, as infix operators generally don't work in chains.
    * For now, the left arg of `->` is a primary, anything else is an error.
      `->` has a low precedence.
      `a b->...` is an error (no currying or chaining).
    * `x->expr`  Special case when there is exactly one argument,
      doesn't work with the Currying abbreviation.
    * Maybe `f(x,y)children=...` is legal; naked identifier legal as final
      curried argument; this abbreviates `f=(x,y)children->...`.
  * syntax: is lambda a chain or an expr?
    * how is this parsed: `a b->expr` ?
      * As `(a b)->expr`, like `a->b->expr`? Expr interpretation.
        Doesn't generalize to `a b c->expr`. Not good. The dual,
        `f a b=expr`, doesn't work.
      * As `a(b->expr)`? Chain interpretation. Consistent with the general
        chain syntax.
      * An error.

## Next Steps: Nonlocal References In A Lambda
What's the next baby step towards supporting non-local references?

Done:
* If the nonlocal is a Constant, the reference is allowed.
  So I can reference builtins.

**Non-recursive closures.**
 1. If a nonlocal is a function parameter, the reference is allowed.
    Now I've got the same power as the lambda calculus.
 2. If a nonlocal is a let or module binding, then I signal an error if the
    binding is *pending* when I try to capture it.

**Recursive static functions.** Constant folding optimization.
Static functions aren't closures: all nonlocals referenced as Constant nodes
or as Code references, see below. They have the same power as C functions.
* Recursive functions can't reference themselves using counted references,
  that's a cyclic reference and storage leak.
  * Can't use a Constant node.
  * Can't use an `RFunction_Ref` node containing a `Shared<Expression>`.
  * Can use a relative branch from inside byte code.
  * Can use an index into the nonlocals array to get an RFunction.
  * Can use an atom to index a "global bindings" dictionary (which we don't
    have in Curv).
  * Can use an index into a top-level-module Code object:
    * Suppose that for every function, the compiled code is owned by a
      particular top-level-module. This compiled code is contained
      in a module_code object which:
      * is constructed at compile time, and contains only VM code,
        no captured nonlocal binding values.
      * can be indexed to get a specific function entry point.
    * The Frame provides access to the current Code object (or there is a VM
      register).
    * To call a static function in the same top-level-module, reference it
      using an index into the current Code object.
    * A Closure value contains a Code pointer, the index of the entry point,
      and captured nonlocal values.

**Recursive Closures.**
The final step, requires RThunks/RFunctions or some new design.
Might not need this for the minimum viable product.

* How would I support access to nonlocal let bindings?
  The current dynamic thunk mechanism would create cyclic references to
  recursive functions, if I allowed functions to be defined recursively.
  The RFunction/RThunk mechanism requires compile time analysis
  of let bindings to detect recursion and mutual recursion.
  * A non-recursive binding is a strict reference to the env vector.
    The value is copied to the closure during construction.

## Nonlocal References (in a function)
A call frame consists of
* `Value* nonlocal;` -- used for non-Constant, non-local lexical bindings,
  which are referenced using compile-time array indexes.
* `Value local[];` -- tail array. Used for bindings that are local to a
  particular function call: arguments, let bindings, other temporaries.
  Referenced using compile-time array indexes.

Three kinds of frame reference nodes:
* `Strict_Local_Ref`: contains a slot index into the `local` array. The slot
  already contains a proper value (ie, not a thunk).
* `Lazy_Local_Ref`: contains a slot index into the `local` array. The slot
  may contain a thunk: if so, then when the Ref is evaluated, the thunk is
  evaluated and the slot is updated with a proper value.
* `Nonlocal_Ref`: contains a slot index into the `nonlocal` array,
  which only contains proper values.

There is a top level call frame used to evaluate a script module,
and a call frame for each function call.

A Closure is the most general form of a user-defined function value.
It contains:
* a code pointer. `Shared<const Expression>`, a shared pointer because
  the same lambda expression may be evaluated multiple times to produce
  different closure values, each with the same code pointer.
* an environment. Maybe `unique_ptr<List>`, or a tail array of Value.
  Contains an array of nonlocal values, referenced by the call frame for
  that function.

A script Module contains an array of slots, referenced by the `nonlocal` pointer
in the Frame used to initialize the Module.

The Closure value for a recursively defined function can't a cyclic reference
to that same value in the environment array. That would cause a storage leak.

## Script Module (without functions or let)
A script module is the only kind of environment. Definitions may refer to each
other, order doesn't matter, there are edge cases where recursion makes sense.
Temporary implementation:
* initialize all binding values to Missing (a special marker value).
* Evaluate definientia expressions one at a time. If they reference a
  Missing value, then:
  * set the slot to Pending
  * evaluate the expression
  * store the result in the slot
* If a definiens slot contains Pending when you evaluate it, then report
  an "illegal recursive reference". Eg, you'll get that for the second `x`
  in `x=x+1`.

Analyzer:
* the Environment contains the builtin namespace and the module namespace
* An Identifier is looked up in the environment. The result is a `Module_Ref`
  (containing an Atom) or a Constant (in the case of a builtin).

Evaluator:
* The eval() virtual function now takes a Frame argument.
  The frame points to the partially initialized script module value.
* `Module_Ref::eval(frame)` works as specified above.

## Environments and Bindings
Kinds of environments:
* function parameter list
* let bindings
* module bindings (script module and submodule)
* the builtin environment

Kinds of bindings and references:
* static -- References are Constant nodes in the code tree.
  True for some let and module bindings, all builtin bindings.
* non-static, non-recursive --
  A references is an index into a run-time slot array, yielding a Value.
  True for all function parameters.
* non-static, recursive --
  We can't just use a slot index, where the slot contains a Value, because
  of reference counting and cyclic references. Instead, the Value is
  represented by an RFunction or RThunk, which must be combined at reference
  time with a pointer to a refcounted environment object (containing a
  slot array associated with a let or module scope).
  * In some cases, the RFunction/RThunk is known at compile time when the
    reference is compiled. That will be true for: let bindings; and
    non-parameter module bindings where the module binding is referenced
    directly and not using the dot operator. So the reference node contains
    the RFunction/RThunk pointer. The environment object is loaded dynamically.
    In the case of a module, the slot array stills need to contain
    the RFunction/RThunk pointer, due to the dot operator.
  * In other cases, the RFunction/RThunk pointer is loaded from a slot and
    discovered at run time.

## References at Runtime
The VM has a current call frame, which contains:
* a tail array of slots, for local references, containing:
  * an argument vector
  * stack of let-binding slot arrays
  * other temporaries, as needed
* `Value* nonlocal`, a slot array for accessing nonlocal values
  * in a script module scope, these are the module slots.
  * when calling a closure, these are the nonlocals in the closure
  * when calling an RFunction/RThunk, these are the slots in the env argument
* debug metadata:
  * function value
  * pointer to previous call frame
  * optional RFunction/RThunk environment pointer?
  * maybe, access to argument locations, to store in bad argument exceptions

To call a Curv function: allocate a call frame for the function (size of frame
is in the function value), store arg values and metadata, call function code.

In the Meaning tree interpreter, the `Meaning::eval` virtual function is passed
the current call frame as an argument. All expressions are evaluated in the
context of a call frame, that's required by the `let` implementation.

Bindings that are local to a call frame are accessed via call frame slots.
Non-local bindings are accessed via the function value (a closure), or if
an RFunction/RThunk is called, then via the environment argument.

The debugger needs a way to inspect the lexical environment when the program
is paused in an arbitrary Meaning phrase. We need the ability to find a
symbol table for the lexical environment at a given meaning, and the ability to
look up run time binding values. It's too much to worry about right now.
* Due to nested `let` expressions, there are different local lexical
  environments for different Meanings within the same call frame.
  A brute force way to get symbols: When the debugger is first invoked,
  traverse the meaning tree and build a map from meaning pointers to symbol
  tables. Or, store an Environ pointer in each Meaning during analysis.

Within a function body, reference node types:
* function parameter: slot index in call frame
* local let binding:
  * static: a Constant node
  * non-static non-recursive: slot index in call frame
  * non-static recursive
    * RThunk: RThunk in reference node, call the RThunk with current call
      frame as environment argument.
    * call to RFunction: call the RFunction with current call frame as
      environment argument, plus arguments.
    * non-call RFunction reference: create closure value from RFunction ptr
      and environment.
* local submodule field reference:
  * Probably, a submodule expression just uses the surrounding call frame
    for let bindings, rather than creating a new frame. So then, we construct
    an uninitialized submodule object and put it into a call frame slot M while
    it is being initialized. During initialization, field references indirect
    through slot M. Afterwards, the submodule may contain thunks and functions
    which get their own call frame when evaluated.
* non-local binding:

Outside a function body, reference node types:

## Code Representation
The fastest path to the Minimum Viable Product now appears to be
a Meaning tree interpreter, using the runtime data structures
I designed for the bytecode interpreter. A faster evaluator is deferred
until later: will consider bytecode interpreter, llvm, webassembly.

## Functions (Meaning Tree Interpreter)

### Representation of Code
If I use a meaning tree interpreter, how does the debug interface work?
In particular, single stepping? Even if I use the tree representation,
I still need a virtual state machine with a PC and registers.
The tree node 'eval' function needs to advance the state.
Worry about this later, once I've worked through the requirements for the
analyzer, which works on the meaning tree.

### Representation of Function Values

Let's start with top level lambda expressions in the builtin context.
How are closures implemented?
```
x->y->x+y
```
A closure consists of: the compile time meaning tree, and the runtime
environment list. The environment list contains non-local bindings which
are non-static. For this particular case, a tail-array representation of
the closure would be efficient.

There are three kinds of environments:
* function parameter list
* let bindings
* module bindings

With the 2nd and 3rd kind, there's a danger of a reference loop,
and the 'rfunction' technique must be used to represent binding values
in those cases. The 'rfunction' technique can't be used in the first case?

Proposal: a closure value contains a code pointer (compile time meaning tree),
an environment pointer, and a tail-array of slots. There are 3 classes
of non-local references:
* static -- converted to Constant nodes in the code tree.
* non-static, non-recursive -- copied into slot array at construction time.
* non-static, recursive -- referenced via environment pointer.

I think there are also 3 classes of function representation:
* static: only need a code tree
* non-static, non-recursive: a closure with slots (see above)
* non-static, recursive: occurs as a let or module binding.
  Only the code pointer is stored in the let/module slot array.
  The function can be called without constructing a closure, because
  non-locals are stored in let- or module- environments. When a function value
  is needed, it's constructed from a code pointer and an environment object.

----------------------------------------------------------------
Static function value:
* Meaning tree (`Function_Expr`?)
  * Compile-time parameter data: array of (parameter name, has default).
* Run-time parameter data: array of default values.

Closure: rfunction and environment.

Modules are functions.

## Functions
I'll assume the bytecode compiler/interpreter here.
All runtime values are boxed, which simplifies things.

When executing a function, the bytecode interpreter needs access to:
* a constant table
* a source map, can just be the Phrase representing the function body.

When compiling a function value to GLSL, we need the Meaning.
That could also be used as a source map.

**Constant Table**
* There is a per-script-module constant table.
  (There could instead be a per-function constant table, but with more
  duplication of values, and no benefit discovered yet.)
* The constant table stores literal constants and the results of folding
  constant expressions, within the module.
* It also stores values of any referenced builtins.
* The constant table should be easily accessible at run time.
  Store it in a register.
  * Each function has a pointer to the constant table of its root module.
    We normally set/restore the register on function call/return.
  * There is a local-call opcode for calling a known function
    within the same root module, that is optimized to not change the
    constants register.
  * Root modules are loosely coupled (no global optimization across them),
    so inter-module calls aren't optimized.

Constants in other modules are referenced by name (late binding).

If we only support a fixed number of boxed arguments,
then the only parameter list description we need is the number of arguments.
Otherwise, if we support optional and keyword arguments in the same way
as OpenSCAD, then we need:
* List of parameter specs. len(list) is # of parameters.
  This list is constructed at compile time.
  A parameter spec contains:
  * parameter name, an atom with pointer equality
  * bool: has default value
  * other parameter metadata that could be added in the future
* List of default values, constructed either at compile time or at run time
  when function value is constructed. `null` is the don't care value.
  (Note, this representation doesn't support default values that depend on
  other parameter values.)

A static function doesn't need to be represented as a closure or rfunction,
because all free variables are compile time constants.
At minimum, we need:
* # of arguments
* Boxed entry point, a pointer to the first bytecode.
* Constant table?

## Modules

An interactive  command line is a script (eg, `a=1;b=2;a+b`).
A `*.curv` source file is a script.

A script denotes a module, and is compiled and evaluated into a module value.

A module constructor `{...}` is a special case with a slightly different
representation and handling: let's call these "submodules".

Top-level modules (script modules) are the unit of separate compilation.
* Currently, this only happens in RAM, there are no compiled object files.
  A Curv session can have multiple scripts loaded. A change to one script
  causes that script to be recompiled, but other scripts referencing it
  do not need to be recompiled.
  * This requires enough late binding so that a module can be modified
    without forcing a recompile of its clients. It prevents global optimization
    across modules, but enables faster recompilation. This tradeoff is likely
    best for an interactive programming environment.
  * Members of script modules can only be referenced by name, not by index or
    pointer (late binding). Better make this efficient.
  * The `builtin` module is a special case, compiled code can depend 
    on its internals.
* Eventually, we can use LLVM to compile script files to optimized shared
  object or DLL files.
* BUT: what are the semantics of `use file M1; use file M2; ...`?
  If there are no duplicate definitions between M1, M2 and ..., then it's
  obvious. But what if a change to script module M1 or M2 creates
  a new definition and makes existing code ambiguous?
  * Under my current design, `use` is evaluated during analysis, and that means
    script module changes *can* force reanalysis of dependent modules.
  * Or, `use` is evaluated at run time. (But, when is that? Any module member
    that depends on a binding B imported via `use` has an RThunk which searches
    for B in all of the `use`d modules.)
  * With these run-time semantics, we might want to change `use` to force
    a more efficient implementation: `use(id1,id2,...)module`.
    * Now, analysis can accurately determine a binding's definition at
      analysis time, and we don't have to search all the used modules on
      each reference to a builtin. We still need to thunk any definiens
      that references an external module binding, though.
    * The implementation of `use` is much simpler with this interface.
      No "Russell's paradox" and no need to implement analysis-time
      evaluation.

Top-level modules are the unit of memory management for compiled code.
You can't keep one function or submodule within a compiled module alive,
while freeing the rest. I think that would be too hard to support while at
the same time permiting global optimization within a single module.
Each compiled module has a reference count. Cyclic dependencies are not
permitted between script modules: an error is reported.
* A script module value owns the Script, the Phrase tree, the Meaning tree
  and the compiled byte code for all contained functions and submodules.
  Function and submodule values contain a counted reference (directly or
  indirectly) to the script module that owns their code.

Do we want to recompile the world each time source is modified?
Or is the world partitioned into separately compiled modules?
There is a runtime performance benefit from global optimization,
and there is a compile time performance benefit from not having to
recompile the world.
* How much will byte code benefit from global optimization?
  Separate compilation and late binding means looking up module members
  by name, rather than using indexes or pointers.
* The source file/module equivalence currently makes scripts the logical unit
  of separate compilation. A user can split code up into modules based
  on this understanding. Eventually, an AOT compiler could compile modules
  to `*.so` files.
* I guess I'd want enough late binding so that a module can be modified
  without having to recompile its clients. The builtin module is a special
  case, code can depend on its internals.

**Fast name lookup.** Ideas:
* Use Lisp-style atoms as identifiers. Name equality is pointer equality.
  Can store hash code inside the atom. Store global atom table in curv::Session.
* At runtime, the dictionary is immutable; it's constructed during analysis.
  For runtime performance, I want fast lookup on an immutable dictionary.
  * Use a sorted array of names. The array is contiguous (good cache locality),
    has few allocations, is easy to address, is queried with binary search
    using std::lower_bound.
* llvm/ADT/StringMap.h: quadratically-probed hash table; each entry is heap
  allocated and contains string key, with string following element.
  Can query a byte range, string is only copied if inserted in table.
  Modify this so that each entry is an atom, use for an atom table.
* llvm/ADT/DenseMap.h: good for mapping pointers (eg atoms) to other pointers.
* std::map: single allocation per pair, log(n) lookup with "extremely large"
  constant factor, space penalty of 3 pointers per pair. Good if keys or values
  are extremely large, you need iteration in sorted order, stable iterators.
* std::hash_map: simple chained hash table. As malloc intensive as std::map.
  Good if elements are huge or comparisons are expensive.

`Script`:
> an array of characters, a command line or source file.

`Module_Phrase : Phrase`:
> an ordered list of statements. A statement is an expression,
> or a definition, or a generator/assert/echo.

`Module_Meaning : Expression`:
* The definitions are converted into a map from names to phrases or meanings.
  (I'm hedging here. The original plan was to keep definientia as phrases
  (for performance reasons, as a JIT strategy), at the expense of not
  reporting analysis errors until later. Obviously this should be benchmarked.
  I don't know if analysis is slow enough for this to matter.)
* The other statements are compiled into a Comprehension expression,
  which when executed, constructs a list of values and performs side effects
  like echo and assert.
* Module parameters are an extra complication.

A `Module_Meaning` is compiled into byte codes, at some point.
My original idea was JIT: don't compile a script module member
until first reference.

=== Module Values
There's a base class `Module`, which is used directly for module values
constructed using `{...}` expressions, and a subclass `Script_Module`,
which represents the script module denoted by a source file.

The base class `Module` contains:
* `slots`:
  A slot array, containing definientia Values, RThunks, RFunctions.
  For a `Script_Module`, definientia may be Phrases (lazy compilation).
  For a SubModule, also contains values of non-local non-static bindings.
  The `use M;` statement introduces a slot value for `M`.
* `dictionary`:
  A shared dictionary mapping names onto slot ids.
  It's a sorted array of atoms, lookup using binary search.
  The atom index is the slot id.
* `elements`:
  A list of Values, initialized by running `elements_thunk` during initial
  construction, and each time the Module is cloned.
* `params`:
  A shared array of slot ids. Or (atom,slotid) pairs if named parameters
  are supported. The length of the array is the number of module parameters.
* `elements_thunk`:
  A shared 1-parameter function (rthunk) for generating the element list
  from the slot array. Also responsible for running echoes and asserts.

A `Script_Module` is the unit of memory management and separate compilation.
Additional resources:
* `constants`: array of Values
* `script`: source code
* `syntax`: the root Phrase
* `meaning`: the root Meaning

Function definitions are represented by RFunctions,
unless the functions are static.

RThunks are used to represent recursive (non-function) definitions,
and definitions that depend on parameters. The RThunk is evaluated each
time the name is referenced.
* The RThunk representation changes the number of times `echo` is executed.
  That's okay, `echo` is a debug feature whose purpose is to expose the inner
  workings of the evaluator.

Design alternatives:
* The `elements` array could be lazily computed:
  * We'd need a separate rthunk for executing the top level echoes and asserts,
    which must be run when the constructor is evaluated (the asserts at least).
  * The side effect ordering might create confusion. Top level assert statements
    would run earlier than top level assert expressions.
  * We'd need a special non-value Value for marking the element list invalid.
    Okay, this isn't hard, and we wouldn't need to touch the Value abstraction.
  * Maybe this is valuable for libraries that include top level shapes
    for demo purposes? Don't want to execute these if just `use`ing the library.
* Cache the results of rthunks for definitions that depend on parameters.
  Store the rthunk in the dictionary, but run it once when
  module is constructed and cache result in slot array. When module is
  customized, run through the dictionary and regenerate these particular
  slots. Extra work and complexity to optimize a particular case.
* For lazy (JIT) compilation, I could store either Phrases or Meanings
  in the slot array. If Phrases, then slightly less work is done up front,
  but semantic analysis errors are reported later, if at all.
* A more complicated `dictionary` might be needed for OpenSCAD scripts.
* Maybe we generate byte code (an RFunction) for customizing a module.
  The top level assert/echo statements are part of the rfunction.
  The rfunction initializes the slots for definientia that depend on parameters
  with regular thunks that are replaced by values on first reference,
  so these can just be lazy instead of evaluated each time.
  Likewise, `elements` can be lazy.
  So a Module ends up with 4 data members: `slots`, `dictionary`, `elements`
  and `customize`.

## Module Operations

`module.id` is implemented by looking up `id` in the dictionary,
getting the slot id, then returning the slot value, with special handling
for phrases, rfunctions and rthunks. (Test the slot value. If an immediate
value return it. Otherwise, get reference value and switch on the type tag,
with special cases for rthunk, rfunction, phrase.)

Similar for `module.id(x,y)`.

`module(arg1,arg2,...)` is implemented by shallow copying the module object.
A `Script_Module` is copied to a `Module`.
The `dictionary`, `params` and `elements_thunk` are shared.
The `slots` array is copied, plugging the argument values into the new slot
array. The `elements` are constructed by evaluating `elements_thunk`.
* Maybe we generate byte code (an RFunction) for customizing a module.

The customizer GUI constructs a copy of the module, then repeatedly { update
a parameter slot, regenerate the element array, preview the elements }.
* Dunno if we want the ability to update the original source file with a
  new parameter value constructed by the customizer. That's possible if we
  keep track of each parameter's definiens' Location.

`use ModuleExpr;` requires ModuleExpr to be an analysis time constant.
This is tricky. Consider:
```
use script("foo.curv");
use script("bar.curv");
```
What happens if either module exports `script`? Or, for that matter, `use`?
This is a logical paradox, similar to Russell's Paradox.

The paradox is avoided by means of scoping distinctions, similar to the
distinct logical levels in Russell's theory of types:
* `use` is a keyword, not an identifier, so it can't be redefined.
* the argument to `use` is resolved in the parent scope.

This design is simple and works. However, it means that this is illegal:
```
foo = script("foo.curv");
use foo;
```

In the implementation, `use M`, where `M` defines `a`,`b`,`c`,
compiles into code equivalent to
```
a = M.a;
b = M.b;
c = M.c;
```
where M is a value stored in the slot array.
