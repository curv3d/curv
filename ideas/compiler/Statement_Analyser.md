## Generalized Definitions
* id = expr
* def1; def2; def3 -- can also contain actions
* ( definition )
* `[x,y,z] = expr` and other patterns
* `if (cond) (a=foo;b=bar) else (a=bar;b=foo)` -- not in MVP

We attempt to recognize a generalized definition in these contexts,
which create scopes:
* bindings argument of `let` and `do`.
* top level phrase in a CLI command. If not a definition, analyse as an
  operation.
* argument of `{}`. If not a definition, analyse as an action/binder sequence.

How are generalized definitions analysed?
* Definitions are recognized during "pre-analysis": `analyse_def()`.
  (The other phrase types are recognized during regular analysis, due to
  metafunctions.)
* The pre-analyser converts a generalized definition into a tree.
  Each tree node lists the bindings defined by that node.
  This is needed to implement conditional definitions (check that the then
  and else branches both define the same set of bindings).
* Can't analyse recursive definitions bottom up. There must be a global
  `Scope` data structure containing the dependency graph.

Data Structures:
* class Definition
  * From the generalized definition parse tree is constructed a Definition tree.
    Definition <: Phrase. Each definition-bearing phrase type either is
    <:Definition, or its analyse_def() function constructs a Definition object.
  * For each binding there is a smallest Definition object (in the tree)
    that defines the binding and contains its definientia.
* Sequential_Scope
* Recursive_Scope
  * A single Recursive_Scope object contains the dependency graph
    for the Definition tree.

Algorithm:

### Phase 1: collect statements
* Construct the Definition tree. `def = phrase->analyse_def(env);`.

### Phase 2: analyse definiens, create dependency graph
```
if (def->kind_ == Definition::k_recursive) {
    Recursive_Scope scope(def);
    scope.analyse(env);
} else {
    Sequential_Scope scope(def);
    scope.analyse(env);
}
```

### Phase 3: generate instruction sequence
* `scope.emit();`


* class Definition represents a tree node, and contains
  * a map from Atom to a slot_t.
  * data for constructing a sequence of slot initialization instructions.
  * an analyse() virtual function for creating the instructions.
  * A compound definition contains intermixed definitions and actions.
    Therefore class Compound_Definition also records the actions,
    its analyse() outputs the action instructions. Sort of. In the recursive
    case, the order of action instructions depends on the global dependency
    graph.

Can't analyse recursive definitions bottom up. There must be a global
`Scope` data structure containing the dependency graph. Its analyse() function
walks all of the definiens in dependency order.
* So how would that work for conditional definitions? The dependencies of
  an 'if' definition are the union of the 'then' and 'else' dependencies.
  In effect, a single binding can have multiple definiens phrases.
* For each binding, there is a smallest Definition object (in the definition
  tree) that defines the binding and contains its definientia.

## New `Statement_Analyser`

This is a new `Statement_Analyser` to fix the storage leak (see Leak.md).

* During analysis, partition a set of `=` definitions so that a set of 1 or
  more mutually recursive definitions are placed in the same "recursive"
  partition. Each non-recursive definition is placed in a singleton
  "nonrecursive" partition.
* All of the definitions in a mutual recursion partition are required
  to be function definitions, or a compile time error is issued.
  (This restriction might be lifted later.)
* Each recursive partition gets its own shared non-locals list, which contains
  only the requirements for that partition.
  The shared nonlocals list contains the Lambda objects for each function.
  It also contains copies of each referenced nonlocal from outside
  of the recursion partition (as proper values).
* Each non-recursive function has a private nonlocals list containing
  proper values.
* The partitions in a definition set are topologically sorted into a linear
  evaluation order, dependees evaluated before dependers.

Inside a recursive function in a mutual recursion partition, references
to group members must use `Nonlocal_Function_Ref`. Outside of that context,
references are just normal slot references to a proper value.

A module contains a list of proper values, one for each field.
This list does not contain nonlocals. To initialize the slots for the
functions in a mutual recursion partition, first construct the group nonlocals
list NL, then construct the closure value for each function using NL.

I don't use thunks. They could be reintroduced in the future.

A block of `=` definitions can be compiled by GL if
there are no recursive functions.

## Runtime Structures

Module values:
* have a `slots_` array which contains only field values (no nonlocals).
* Field slots are initialized with proper values (not thunks).
* Field slots are initialized by executing a sequence of actions,
  one action per recursion partition. These actions use the module's parent
  Frame to access nonlocals. This design doesn't support module customization,
  since that Frame isn't available at customization time.
* Closures in the slots array do not point to the slots array (unlike now).
  Each field value that's a closure has a nonlocals list.
* The initialization action for a nonrecursive field initializes that field.
  If it's a function, a private nonlocals list is constructed.
* The initialization action for a recursion group initializes all of the fields
  in that recursion group, first constructing the nonlocals list NL shared by
  the functions in the recursion group. In NL, the recursion group member slots
  are immutable and contain only lambdas, never values.

Blocks:
* Each binding has a direct slot. Indirect slots are not used by blocks.
* A binding slot is initialized by the initialization action for its
  mutual recursion partition.

## Environment Lookup

Currently, we assign a slot# to definition-bound identifiers
during environment lookup. This will continue to be possible.

When we look up an identifier in an environment, if it is definition-bound,
then we analyse the definiens, and we recursively analyse its references.
Once this analysis is done, the rank and type of the definition's mutual
recursion partition is established, and we can assign a slot #.

Currently, `Statement_Analyser` uses these reference types:
    if (kind_ == Definition::k_recursive) {
        if (b->second.is_function_definition())
            make<Indirect_Function_Ref>
        else
            make<Indirect_Lazy_Ref>
    } else { // sequential
        if (target_is_module_)
            make<Indirect_Strict_Ref>
        else
            make<Let_Ref>
    }
With the new design, all references are strict:
    if (target_is_module_)
        make<Indirect_Strict_Ref>
    else
        make<Let_Ref>

## Partition Definitions into Recursion Groups

Given a set of recursive definitions, partition them into mutual recursion
partitions, labeling each partition as recursive or nonrecursive.

Assign a sequence number to each partition (position in the evaluation order).
Action statements also have sequence numbers. Action statements are executed
in source-code order, and are executed as early as possible (as soon as their
dependency definitions are executed), so that assertions can guard later
definitions.

In a recursive statement list, we will analyse the action statements first.
The output is a mixed sequence of statement action ops and (as a side effect)
MR Partition action ops. Then we analyse the definientia of the remaining
unanalysed definitions, which will output the remaining MR Partition action ops.

A set of recursive definitions is a directed graph, and a mutual recursion
partition is a "strongly connected component" of that graph.
https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm

Data structures:
* Here's the additional state associated with each Definition in a
  `Statement_Analyser`. Each Definition is:
  * not analysed;
  * analysis is in progress;
  * analysed and {recursive | not recursive}.
* An `MR_Partition` contains a list of `Shared<Definition>`.
  Probably don't need the definition to also point to the MR partition,
  that keeps the data structure hierarchical.
* A `Statement_Analyser` contains a list of `MR_Partition`.
* `Statement_Analyser` contains temporary state used during definition analysis:
  * The current `MR_Partition`, or null.
  * A stack of definitions.

Analysis:
* Pick an unanalysed definition D from a `Statement_Analyser` SA.
* Set the analysis state of D to "in progress".
* Set the current `MR_Partition` to null, and the defstack to D.
* Begin analysing the definiens of D.
* Within `Statement_Analyser::single_lookup`, there is a match M.
  * Prepare to analyse the definiens of M.
  * If M is already under analysis, then we have discovered recursion:
    * Mark M as recursive.
      * If the SA doesn't have a current `MR_Partition`, create a new one.
      * Add M to the current `MR_Partition`.
      * Suppose we visit `z->a->b->c->a`, and discover that `a` is recursive.
        Then `b` and `c` must be added to a's `MR_Partition`. How is that done?
      * The SA contains an explicit representation of the DefRef analysis stack,
        along with the current `MR_Partition`.
      * When recursion is detected, we mark all of the stack elements back to
        the recursion point as recursive, and add them to the current partition.
  * Otherwise, analyse M:
    * Push M onto the DefRef analysis stack.
    * Mark M as under analysis.
    * Call analyse() on M's definiens.
    * After analyse() returns, if M is still marked as "under analysis",
      then mark it as analysed + nonrecursive.
    * Pop the DefRef stack.
  * Finally, return a `Def_Ref` containing M.

## `Statement_Analyser` Analysis Phases

**phase 1, collect statements:** `Statement_Analyser::add_statement`

* right now, we collect actions and definitions into a data structure:
  * `action_phrases_`, a list of {seq_no, phrase}.
  * `def_dictionary_`, maps identifiers to {seq_no, definition}.
  * actions and sequential definitions have a sequence number, recursive
    definitions all have seqno -1.
* This is good enough for now.

For a recursively defined lambda, we also set `shared_nonlocals_` to true.
That still makes sense.

**phase 2:** `analyse` and `Thunk_Environ::single_lookup`

This will be rewritten.
Use different algorithms for sequential and recursive statement lists,
so the sequential code doesn't change.

**data structures**
* `actions_` vector of struct Analysed_Action {
      int seq_no_;
      Shared<const Phrase> phrase_;
      Shared<Operation> op_;
      int op_index_;
  }
* `bindings_` maps Atom to struct Binding {
      int definition_index;
      int binding_index;
  }
  * a definition may bind N names, each has an index in `0..<N`.
* `definitions_` vector of struct Analysed_Definition {
      Shared<const Definition> def;
      Shared<Operation> definiens_;
      Module::Dictionary nonlocals_;
      int partition_index; // -1 until partition assigned
      int seq_no;
      slot_t first_slot;
      enum class State {not_analysed, analysis_in_progress, analysed} state;
  }
* `partitions_` vector of struct MR_Partition {
      std::set<int> definitions;
      int op_index_;
  }
  initially empty, grows as definitions are analysed.
* `defn_stack_`: Stack of definitions currently being analysed.

**`analyse`**

Currently, for recursive statements lists, we:
* allocate the actions array, set all elements to null.
* analyse the action phrases, plugging results into the actions array.
* analyse each definition using a `Thunk_Environ`, then construct a
  Thunk or Lambda pseudo-value and store it in defn_values.

In the new design,
* Initialize the actions array to empty.
* For each action phrase, analyse it to an action op A.
  As a side effect, some more MR partitions will be created.
  Convert those MR partitions to action ops, and append them to the action list.
  Then append A to the action list.
* Analyse each definiens which hasn't already been analysed.
  Convert remaining MR partitions to action ops, append them to the action list.

**`single_lookup`**

Recursive statement lists currently use an indirect value list stored in
the frame, and thunks. `Thunk_Environ::single_lookup` is used to analyse
actions and definientia: this returns `Nonlocal_Function_Ref` or
`Nonlocal_Lazy_Ref`.

In the new design, we use `Lambda_Environ` to analyse a `Lambda_Phrase`
definiens. In other cases, `Statement_Analyser` is the environ class.

* The symbol lookup fails, or resolves to binding B.
* Before constructing a Ref, we recursively analyse the definiens of B.
  `analyse_definiens(B->definition_index_)`
* Outside of a function definition,
  a reference is `Indirect_Strict_Ref` in a module, `Let_Ref` in a block.
  We can assign a slot number when the Ref is created.
* Inside of a function definition, a reference is a `Nonlocal_Symbolic_Ref`.

  For efficiency, we'd prefer to use a `Nonlocal_Function_Ref` or a
  `Nonlocal_Strict_Ref`. But at Ref creation time, we can't assign a slot
  number, because we might later merge two function definitions into the
  same MR partition.
  * Initially, the evaluator uses an Atom to look up nonlocals.
    We'll change the representation of a nonlocals list from List to Module.
  * Later, an optimization pass will convert symbolic refs to slot refs.

  The Environ also builds a nonlocals dictionary for each function definition.

Symbolic nonlocal refs are the most disruptive issue.

**`void analyse_definiens(int defindex)`**

```
  if (D->state == analysed) return;
  if (D->state == analysis_in_progress) {
    // We have discovered recursion.
    // Currently, only function definitions can be recursive.
    // The GL compiler needs to distinguish
    // recursive vs nonrecursive functions, so record this.
    ..
    return;
  }
  assert(D->state == not_analysed);
  D->state = analysis_in_progress;
  push D onto the defn_stack_;
  create a partition for D;
  auto lambda = cast<const Lambda_Phrase>(D->definition->definiens);
  Shared<Operation> op;
  if (lambda) {
    Lambda_Environ env(...);
    op = analyse_op(env);
    merge nonlocals dictionary into the partition;
  } else {
    op = analyse_op(*this);
  }
  // This algorithm doesn't tell us when an MR partition is complete.
  // So we don't create MR partition actions right now; that happens at a
  // higher level. Instead, we store the op in the definition stmt.
  D->op = op;
  if (D->state == analysis_in_progress) {
    // non-recursive case
    D->state = analysed;
  }
  pop the definition stack;
```

* Set the analysis state of D to "in progress".
* Set the current `MR_Partition` to null, and the defstack to D.
* Begin analysing the definiens of D.
* Within `Statement_Analyser::single_lookup`, there is a match M.
  * Prepare to analyse the definiens of M.
  * If M is already under analysis, then we have discovered recursion:
    * Mark M as recursive.
      * If the SA doesn't have a current `MR_Partition`, create a new one.
      * Add M to the current `MR_Partition`.
      * Suppose we visit `z->a->b->c->a`, and discover that `a` is recursive.
        Then `b` and `c` must be added to a's `MR_Partition`. How is that done?
      * The SA contains an explicit representation of the DefRef analysis stack,
        along with the current `MR_Partition`.
      * When recursion is detected, we mark all of the stack elements back to
        the recursion point as recursive, and add them to the current partition.
  * Otherwise, analyse M:
    * Push M onto the DefRef analysis stack.
    * Mark M as under analysis.
    * Call analyse() on M's definiens.
    * After analyse() returns, if M is still marked as "under analysis",
      then mark it as analysed + nonrecursive.
    * Pop the DefRef stack.
  * Finally, return a `Def_Ref` containing M.

**phase 3: Generate Action List**

During the analysis phase, we analysed all of the definition and action
statements, and accumulated all of the data needed for phase 3. We know how many
action ops there are (sum of action statement count and MR partition count).
We've assigned an action index to each action statement and MR partition.
