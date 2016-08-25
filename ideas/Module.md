# Implementation of Functions and Modules

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
