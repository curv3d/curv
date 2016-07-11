# Implementation of Functions and Modules

## Static Expressions
An expression is static if all of its free variables are static:
* Builtin bindings like `true` and `sqrt` are static.
* A function formal parameter is not static.
* A `let` bound variable is static if its definiens is static.
* A parameter binding of a module is not static.
* A non-parameter binding of a module is static if its definiens is static.
* In the case of recursive definitions within a `let` or module,
  we tentatively assume that all non-parameter bindings are static,
  then attempt to disprove this by finding non-static free variables
  in the definientia. If no such non-static free variables are found,
  then the binding is considered static.

Staticness is a property of expressions which is computed by the
semantic analyzer. It's required by the `use` operator.
So curv::Expression has an `is_static_` member, which we can compute
in a mostly bottom up manner.

As a performance optimization, static expressions are compile time constants,
and are computed exactly once. I want to be careful that the cost of
constant folding doesn't exceed the performance benefit.
* In the case of animation, the entire script is being evaluated perhaps
  hundreds of times.
* In another case, you interactively make changes to a script, evaluate it once,
  then iterate. The entire cost of compilation is currently incurred when you
  press F5. It would be better if compilation took place in the background
  during editing, and if we cached the results of compiling parts of the
  script so that we didn't have to recompile the world from scratch each time.
  One easy fix is to cache the results of compiling a used module.

How is compile time evaluation implemented,
and when does compile time constant folding actually take place?
* I want to replace the Meaning tree evaluator with a byte code compiler
  and evaluator. The latter is faster than the tree evaluator if the same
  subexpressions are evaluated multiple times. Plus, eliminate code duplication.
* So it makes sense to use the byte code evaluator to reduce static
  expressions.
* If I do this strictly bottom up, then 2+3+4 will result in 2+3 being
  compiled to byte code, evaluated to 5 and placed in a Constant node,
  followed by 5+3 being compiled to byte code, evaluated, etc.
  So this could be quite inefficient.
* If I do this strictly top down, then we find that a script file, as a whole,
  is a static expression (even if many of the subexpressions aren't static).
  Top down constant folding just means compiling a script file to byte
  code and evaluating it. This doesn't ensure that static expressions
  are computed exactly once.
* Constant folding is a performance optimization. It has a cost, and isn't
  worth doing unless the expression being folded is evaluated more than once.
* So I need a variation of top down where subexpressions that could be
  evaluated more than once are folded first.

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
