This is leaky:
{
reduce(first,f) rest =
    if (rest==[]) first else reduce(f(first,rest'0), f) << rest'(1..<len(rest)),
sum = reduce(0, (x,y)->x+y),
}

This is not leaky:
{
reduce(first,f) rest =
    if (rest==[]) first else reduce(f(first,rest'0), f) << rest'(1..<len(rest)),
sum list = reduce(0, (x,y)->x+y) list,
}

A simple example of a leak:
{
f x = x,
a = f,
}
The reference `f` is promoted to a closure which points to the module.
This closure is stored in the module slot for `a`, creating a reference cycle.

## Analysis 1
Both examples involve partially applied recursive functions which are stored
in recursive bindings of the same scope, creating a cyclic reference.

What to do?
* Initially, rewrite curv code that leaks memory. Get the tests passing
  with -fsanitize=leak enabled.
* Then, get to the point where leaking code patterns are detected and
  generate an error.
* Then generalize the implementation so that fewer code patterns leak and
  more patterns are accepted.

### Approaches to an Ideal Solution

Dependency analysis.
* In a set of `=` definitions with no recursive dependencies,
  we can compile function definitions as functions with private
  (not shared) nonlocals lists. Then the whole problem goes away.
* During analysis, partition a set of `=` definitions so that a set of 1 or
  more mutually recursive definitions are placed in the same "recursive"
  partition. Each non-recursive definition is placed in a singleton
  "nonrecursive" partition.
* Each recursive partition gets its own shared non-locals list, which contains
  only the requirements for that partition.
* If a non-recursive definiens contains a reference to a recursive binding,
  it is safe to evaluate that reference to a proper value (eg, a closure
  containing the nonlocals list).
* Ditto if a recursive definiens contains a reference to a recursive binding
  in another partition.
* But, if a recursive definiens contains a reference to another recursive
  definition in the same partition, then special handling is required.
  * Conceptually, the shared nonlocals list is available at runtime when
    interpreting the reference, and that's what is used to resolve the
    reference. If that isn't possible, then it is an illegal recursive
    reference.

We still have the same bug, but now we've reduced the scope of the bug
to pairs of mutually recursive definitions.

Perhaps all of the definitions in a mutual recursion partition are required
to be function definitions, or a compile time error is issued. Good enough.

The shared nonlocals list contains the Lambda objects for each function.
It also contains copies of each referenced nonlocal from outside
of the recursion partition.

Each nonlocals list needs to be initialized before any of the definitions
in a recursion partition are evaluated.
The partitions in a definition set are topologically sorted into a linear
evaluation order, dependees evaluated before dependers.

Inside a recursive function in a recursion group, references to group members
must use Nonlocal_Function_Ref. Outside of that context, references are just
normal slot references to a proper value, no thunks or lambdas required.

So a module contains a list of proper values, one for each field.
This list does not contain nonlocals. To initialize the slots for the
functions in a recursion group, first construct the group nonlocals list NL,
then construct the closure value for each function using NL.

So I don't need thunks. Expressiveness is the same as now (May 2017), less
than Haskell. It's good enough. This scheme might be easier to integrate
with sequential definitions and GL than the thunk based design.

### Implementing Dependency Analysis

Given a set of recursive definitions, partition them into recursion groups,
label each partition as recursive or nonrecursive, topologically sort the
groups into a linear evaluation order.


## Analysis 0
reduce is a curried, recursive function.
* The inner lambda is considered non-recursive.
  It captures the 'reduce' closure in its private nonlocals list.
* 

Hmm.
1. sum is not considered a function definition. So when we force the sum slot,
   we call reduce.
2. reduce is considered a function definition, so its slot contains a Lambda,
   which forces to itself.
3. The reduce call (1) evaluates reduce to a closure, and its arguments to
   a list, then applies closure to list.
4. eval reduce: it's curried, so nested lambdas. The closure contains the
   outer lambda and the module as nonlocals.

module slots:
 0: reduce: outer Lambda
 1: sum: result of reduce call
     call(Closure(outer Lambda, module), List(0,plus_lambda))
      Frame 0: 0, 1: plus_lambda
      eval body of outer lambda
       eval inner lambda
        capture reduce, first, f
        result from reduce call is a closure containing
        Closure(outer Lambda,module), and this is stored in the sum module slot.
        -> which means, reference loop and storage leak.

How to fix this?
(a) No recursive curried functions. Brutal.
(b) Special representation for recursive curried functions.
    * In `sum = reduce(0,(x,y)->x+y)`, the reduce call returns some entity R
      that doesn't contain a module pointer, since R is stored in the module
      slot for `sum`, and we can't have a reference loop.
    * R is not a proper value, it's interpreted relative to the module.
    * An entity like R is only valid in certain expression contexts.
      * Eg, not in this context: `reduce=..; a=[R];` since if R is promoted
        to a full value in this context, then we'll have a reference loop.
      * Nothing special about `reduce` or partial applications. Any
        recursive function has the same problem. `f x=x;a=[f];` is a leak.
    * We notice that R is not a proper value:
      * At compile time.
      * At run time.

    * An N-deep curried function is treated as an N-ary function.
    * There is an op for N-ary curried function call.
    * Partial application creates a Partial_Application closure.
      Locals and nonlocals are captured separately...
    * Outside of this framework, a recursive ref inside a lambda is an error.

    is treated as a special case that creates a Partial_Application closure.
     * Okay, but what does the closure look like? It has to capture nonlocals
       outside of the function, and also capture locals.
    A recursive reference inside a lambda expression, outside of this framework,
    is an error.

----------------------------------------------------
5 leaks reported:

1. Shared<Closure> : Nonlocal_Function_Ref::eval : Just_Expr::generate
   : List_Expr::eval_list : Lambda_Expr::eval : Closure::call
   : Polyadic_Function::call : Call_Expr::eval : force : Statements::eval.535
   : Module_Expr::eval_module
the `reduce` closure in the definiens of sum.

2.
#0 __interceptor_malloc (/usr/lib/x86_64-linux-gnu/liblsan.so.0+0xc795)
#1 Shared<Closure> make<Closure, Shared<Operation> const&, Shared<Tail_Array<List_Base> >, unsigned int const&, unsigned int const&>(Shared<Operation> const&, Shared<Tail_Array<List_Base> >&&, unsigned int const&, unsigned int const&)
   aux/shared.h:51
#2 Lambda_Expr::eval(Tail_Array<Frame_Base>&) const evaluator.cc:659
#3 Just_Expression::generate(Tail_Array<Frame_Base>&, List_Builder&) const evaluator.cc:34
#4 List_Expr_Base::eval_list(Tail_Array<Frame_Base>&) const evaluator.cc:470
#5 List_Expr_Base::eval(Tail_Array<Frame_Base>&) const evaluator.cc:477
#6 Call_Expr::eval(Tail_Array<Frame_Base>&) const evaluator.cc:131
#7 force(Tail_Array<List_Base>&, unsigned int, Tail_Array<Frame_Base>&)
   thunk.cc:62
#8 Statements::eval(Tail_Array<Frame_Base>&) const
   evaluator.cc:535
#9 Module_Expr::eval_module(Tail_Array<Frame_Base>&) const
   evaluator.cc:564
#10 Module_Expr::eval(Tail_Array<Frame_Base>&) const
    evaluator.cc:557
#11 Program::eval() program.cc:78
