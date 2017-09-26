This is a new `Statement_Analyzer` to fix the storage leak (see Leak.md).

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

Inside a recursive function in a recursion group, references to group members
must use `Nonlocal_Function_Ref`. Outside of that context, references are just
normal slot references to a proper value.

A module contains a list of proper values, one for each field.
This list does not contain nonlocals. To initialize the slots for the
functions in a recursion group, first construct the group nonlocals list NL,
then construct the closure value for each function using NL.

I don't use thunks or indirect slots.

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
* Each binding has a direct slot. No more indirect slots.
* A binding slot is initialized by the initialization action for its
  recursion partition.

## Environment Lookup

Currently, we assign a slot# to definition-bound identifiers
during environment lookup. This will continue to be possible.

When we look up an identifier in an environment, if it is definition-bound,
then we analyze the definiens, and we recursively analyze all of the
definiens' references, all the way down. Once this analysis is done, the
rank and type of the definition's recursion partition is established, and
we have all the information we need to assign a slot #.

Currently, `Statement_Analyzer` uses these reference types:
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

Given a set of recursive definitions, partition them into recursion groups,
labeling each partition as recursive or nonrecursive.

Assign a rank to each recursion group (position in the evaluation order).

https://en.wikipedia.org/wiki/Tarjan%27s_strongly_connected_components_algorithm

Data structures:
* Here's the additional state associated with each Definition in a
  `Statement_Analyzer`. Each Definition is:
  * not analyzed;
  * analysis is in progress;
  * analyzed and {recursive | not recursive}.
* A `Recursion_Group` contains a list of `Shared<Definition>`.
  Probably don't need the definition to also point to the recursion group,
  that keeps the data structure hierarchical.
* A `Statement_Analyzer` contains a list of `Recursion_Group`.
* `Statement_Analyzer` contains temporary state used during definition analysis:
  * The current recursion group, or null.
  * A stack of definitions.

Analysis:
* Pick an unanalyzed definition D from a `Statement_Analyzer` SA.
* Set the analysis state of D to "in progress".
* Set the current recursion group to null, and the defstack to D.
* Begin analyzing the definiens of D.
* Within `Statement_Analyzer::single_lookup`, there is a match M.
  * Prepare to analyze the definiens of M.
  * If M is already under analysis, then we have discovered recursion:
    * Mark M as recursive.
      * If the SA doesn't have a current recursion group, create a new one.
      * Add M to the current recursion group.
      * Suppose we visit `z->a->b->c->a`, and discover that `a` is recursive.
        Then `b` and `c` must be added to a's recursion group. How is that done?
      * The SA contains an explicit representation of the DefRef analysis stack,
        along with the current recursion group.
      * When recursion is detected, we mark all of the stack elements back to
        the recursion point as recursive, and add them to the current recursion
        group.
  * Otherwise, analyze M:
    * Push M onto the DefRef analysis stack.
    * Mark M as under analysis.
    * Call analyze() on M's definiens.
    * After analyze() returns, if M is still marked as "under analysis",
      then mark it as analyzed + nonrecursive.
    * Pop the DefRef stack.
  * Finally, return a `Def_Ref` containing M.
