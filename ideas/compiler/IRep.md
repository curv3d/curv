# IR (Intermediate Representation) 2018
This is a huge project that comprises both a more powerful GL compiler,
and a more efficient interpreter. It is too big to digest in one piece.
For just the GL compiler part, see `New_GL_Compiler`.

## Nov 2017: New Compiler
Primary goal: to expand the subset of Curv useable in GL code.
* Call a non-const function, to support animation.
* Top level data definitions, to support parametric sweeps.
* GL optimizations like constant folding, common subexpression elimination,
  algebraic identities/strength reduction. Abstract interpretation of non-GL
  sub-expressions into GL-compatible expressions (eg, break down compound
  arguments, partial evaluation eliminates non-GL values).
* GL: multiple top level data and function definitions.
  Don't necessarily inline expand all function calls.

Longer term goals:
* GL: convert tail recursion to iteration.
* interpreter: constant space tail recursion
* debugger: single stepping
* uniqueness optimizations (in place update of unique objects, COW), based on
  refcount checking and move optimizations.

**New Intermediate Representation:**
* The new IR is similar to/an evolution of the Operation class.
* Phrases are compiled into IR trees during analysis.
* The IR is then either (a) interpreted directly, or (b) compiled into an
  executable representation called Instructions.
* The IR phrase type (action, expression, list gen, record gen) is determined
  during analysis and frozen when the IR node is created. Rationale:
  * Better error messages during analysis time (wrong phrase type in this
    context).
  * If IR can be interpreted directly, *and* if the interpreter supports
    constant space tail recursion, then there is a single virtual exec()
    that updates the interpreter state and returns the next instruction.
* IR nodes are kind of like meta-values or abstract values, which are subject
  to abstract (compile time) interpretation. Examples of IR node classes:
  * Constant values
  * Abstract Lists, of various sorts, depending on the amount of compile time
    info. It's a List expression. We may know the # of elements, with an
    IR expression for each element, in which case `count` can be evaluated
    at compile time, etc.
  * Abstract Records, Dicts, Modules, Strings, Numbers, Functions.
  * Function calls, field refs, all the expression types.
  * A Data Ref:
    * Has an associated IR definiens expression (for the immutable `=` case).
      Needed for abstract evaluation.
    * Is like a reference to an SSA variable.
    * We don't assign slot numbers during analysis, because SSA variables can be
      created and deleted during optimization/abstract evaluation, after
      analysis is complete.
    * Are SSA variables organized into nested scopes corresponding
      to 2017 Scopes? Or are they global to a call Frame, and we perform
      lifetime analysis and register allocation during code gen?
    * References to non-recursive definitions are represented by shared
      references to the definiens. The IR graph is a DAG, a structure
      which also supports common-subexpression elimination. Pointer equality
      of IR nodes means optimization-time equivalence.
  * A block whose body is a list constructor is analysed into something
    that looks like an abstract List to the IR abstract evaluator.
    * Maybe it's an IR_List_Expr, and all of the statements/actions associated
      with the block get shoved into the IR_Frame somehow.
    * Or maybe it's an IR_Block_Op, and the listiness is represented by an
      IR_Type field, subclass IR_List_Type.
* IR expression nodes have GL type fields.
* During GL compile, we take a Shape value S, and convert that into an IR
  constant expression SX. Then we construct the IR expression `SX.dist(px)`,
  and transform that into a GL compatible IR expression, by inline expanding
  function calls and performing abstract interpretation. We also generate a
  set of top level GL definitions, since we now support global variables
  containing const data structures, and noninlined function definitions.
* Once the GL IR is generated and optimized, then we convert that into GLSL.

**EXE (Executable Representation):**

* The interpreter is a state machine with a main loop and an IP (instruction
  pointer) register, storing all temporary values in the Frame. Supports
  constant space tail recursion, single stepping. Minimal use of C stack.
* Instructions for strict operands are executed, and their results stored in
  the frame, before the operator instruction is executed.
* Instructions are Operation objects. An expression has a `slot_t result_`
  field, indicating where the result is stored. It has zero or more shared
  pointers to its operands. It uses `operand->result_` to fetch the value of a
  strict operand, which was previously executed.

```
// Execute an instruction, using Frame slots for input and output values.
// Update the IP and possibly the frame register to set up for next instruction.
Operation* Operation::iexec(unique_ptr<Frame>&) const

Operation* IP = program->body_;
unique_ptr<Frame> frame = Frame::make(program->nslots_, ...);
while (IP = IP->exec(frame))
    ;
```
Most operations have a `Operation* next_` field which is initialized during
codegen().

**Changes**<br>
Existing GL compiler is refactored:
* Inline expand a call to a closure value, yielding IR (an Operation tree).
* IR to IR optimizations (new)
* Convert an IR tree to GLSL, similar to original GL compiler code.

**First Step: GL animation**<br>
I want to call animate() from GL. Animate has an anonymous lambda expression
as argument, which may capture local variables. Want to inline expand calls
to those lambdas. I want to perform inline expansion of function calls using
an IR to IR transformation. The function could be a Closure value or it could
be a Lambda expression.
* Top level shape S compiled to GLSL. Create a Lambda expression for the
  GLSL `main_dist` function. The body is a Call expression with S.dist (wrapped
  in Constant) and the parent lambda argument (an `Arg_Ref`).
  Then inline expand the body, IR to IR.
* To inline expand a Call expr, several cases.
  * Lambda expr. Just copy the body, replacing arg refs with the actual
    argument.
  * Closure value. Copy the body, replacing arg refs and nonlocal refs with
    the corresponding expression.
  * Non-closure Function value. Leave the call expr unchanged. Maybe call
    `call_optimize()` on the function value so that it can perform constant
    folding or partial evaluation.

`AFrame` is an abstract or analysis/optimization time representation of
a frame. It contains an Operation hash table. When creating a new Operation
within a frame, check if it already exists in the table. This merges common
subexpressions, creates a DAG of operations.

**Older Notes**<br>
New GL IR, replaces `GL_Value`.
It's an updated version of Meaning/Operation/Expression, in which a variable
reference links to the definiens. We support IR to IR transformations, such as
constant folding, abstract/partial evaluation, common subexpression elimination.

Phrase::analyse() creates a Meaning/Operation, but frame slots aren't allocated
yet. Then we optimize the IR. Then we generate code (which includes slot
assignment). The GL compiler generates new IR, which is optimized and translated
to GLSL.

First step is to modify Operation so that a variable reference links to the
definiens. BUT we can't have reference cycles in the Operation graph.
* We handle recursive and non-recursive definitions differently.
* A reference to a non-recursive variable is the definiens. Multiple
  references to the same variable are shared pointers to the same Expression.
  The use-count indicates how many references to the variable exist.
  Common-subexpression optimization can result in the same situation.
  In the end, it's the existence of common subexpressions, not variable
  definitions, that controls frame slot allocations.
* But, that conflicts with keeping a meaning tree (not DAG) with a syntax_
  field in each tree node? It seems that for non-recursive variables, we won't
  have operations with Identifiers as source. If such an identifier is an
  argument to a function call, it won't be underlined in an error message?
  Actually that's no problem, since that's done by walking the Call_Phrase.
  Maybe this is okay.
* But, how does this affect the interactive debugger, examining local variables?
* Recursive definitions are different. A module value M is constructed,
  a reference becomes an (M,slot) pair. Probably the slot isn't allocated until
  codegen time.

## Mar 2017
The main thing I need now is a more powerful GL language and compiler.
* Blocks are supported. Currently, `let` is used instead.
* Some kind of while-loop iteration is supported. Ideally, tail recursion is
  supported. Or some kind of bespoke loop construct that is easier to compile.
  `loop sum(i=0,total=0) if (i < len list) sum(i+1,total+list[i]) else total`

I need to compile GL into an IR, optimize the IR, then finally convert the IR
to GLSL. The IR can contain intermediate expressions that aren't GLSL
compatible, which is okay if they are constant folded or transformed into
something that is compatible.

## Feb 2017
1. Phrase tree is compiled into an "executable IR", currently an Operation tree.
   Optimizations at this stage:
   * tail call optimization
   * uniqueness optimization for `update(i,x)struct`
   Don't need a full optimizing compiler at this stage. No constant folding
   unless I have a *semantic* need for it. LuaJIT waits until a performance
   problem is detected before optimizing anything. My StupidJIT™ is applied
   manually to perf.critical functions.
2. GL compiler performs optimizations.
   * common subexpression elimination
   * constant folding
   * (a*b)+c -> FMA(a,b,c)
   * min[inf,x] -> x // for union.dist
   * if(c)x else x -> x // for union.colour

The new XIR supports:
* single stepping
* IR->IR transformations?
* GL compiler can be retargeted for LLVM, for StupidJIT™ or polygonization.

**Executable Format** supports single stepping. Each instruction gets its
strict arguments from local frame slots, stuffs its result into another local
frame slot, then jumps to its continuation. The C stack isn't used for temp
values anymore.
* If_Op has 2 continuations.
* `a && b` needs to be compiled into two instructions. One tests `a` (which is
  strict) and jumps to one of 2 continuations.
  The other coerces the result of `b` to boolean.

The `&&` op suggests that the executable format is not quite an IR? I was hoping
it would be isomorphic to CPS. More analysis later.

**GL IR**:
* GL_Value/GL_Frame/gl_eval: not useful for optimizations. I can't test if
  a GL_Value is constant, so no constant folding.

## Jan 2017
Goals:
* A function can return multiple values.
  The number of values returned by a given lambda is fixed at compile time.
  The number of values returned by a given function call expression is
  fixed at compile time. (17,42) is an expression that yields 2 values.
  As a special case, a function can return 0 values (funcall is an action).
  This will help with generating fast GLSL code. Distance functions will
  return 2 values, a distance and a colour.
  * An expression that yields N values has N slots assigned at compile time
    to hold those values. Expressions read values out of slots and store
    results in slots.
  * A function that returns N values has N slots dedicated to its return values
    in its call frame.
* Tail call optimization. You can only make a tail call to a function with
  the same # of results. Your stack frame is replaced by the stack frame for
  the function you are calling -- the results have the same offsets, they are
  at the beginning of the frame. When a function call returns in the normal
  way (after a non-tail call), the caller has access to the final resulting
  stack frame, and pulls the results out. So, with my current scheme of
  individually heap-allocated frames, the callee must own the frame, and
  on return, ownership is returned to the caller. Or I could have one big
  contiguous stack.
* Single stepping. VM is a state machine and the debugger can advance the state.

Soft Goals:
* Uniqueness optimizations. Eg, `update(i,x)struct` can reuse struct if it
  has use_count==1 and this is the final use of the value in this slot.
  Argument values are moved into a new stack frame, not copied, if there are
  no further references to the arg values from the parent frame.
  This is low priority, but I'd like to be making progress towards this.
* The compiler uses an IR intermediate representation which can be optimized
  using IR->IR transformations. Maybe the same IR works for both conversion
  to VM Code and conversion to GLSL. Want SSA-type optimizations like constant
  folding and common subexpression elimination to benefit GL.
  An SSA variable knows how many references it has. If one ref, then we
  don't need an intermediate variable in the GLSL output. The last reference to
  a given SSA variable is marked as such, the value can be moved out of the
  slot instead of copied.

Currently, an Expression has argument subexpressions. The `eval` method
evaluates the subexpressions using the C stack, then operates on the resulting
values. (It's a tree interpreter.)

In the new VM Code, an op node references argument slots which contain
already computed values. The result is computed, stored in slots.
The single stepper can execute single instructions, advancing the instruction
pointer. Control structures like &&, || and if work differently, they use
conditional branch instructions.

So, structure of the VM code:
* byte code. Where do you put the Location data?
* threaded code. Maybe an array of object pointers, where each Operation object
  contains arbitrary data plus a function pointer or a vtable. This is
  not designed for speed, but is very flexible and close to how my current
  system works.
* Maybe a linked list of Operation objects. Now even closer to my current
  Operation tree.
  * Sounds kind of like CPS. For example, a binary+ node would contain
    two slot #s (aka CPS variable names), plus a "continuation"
    (like a lambda used as a continuation argument), which would contain
    a "parameter name" (slot # for the result) and a "lambda body"
    (pointer to the next node to be executed).

If I don't care about efficiency, maybe I can use the same representation
for IR and VM Code. Not sure, since the two formats have different requirements.
Two main IR choices: SSA and CPS.

Okay, I think maybe I can use CPS as both my IR and my VM code.

## CPS as IR and VM Code

What happens to the Operation class in this new world?
* An Expression produces zero or more results; the number of results is
  known at compile time. nresults_ is a data member. These results are passed
  as arguments to the Expression's continuation. An Action is now just
  an Expression that produces zero results.
* A Generator produces zero or more results; the number of results is not
  in general known until run time. The results are appended to a List_Builder.
* We now determine the Expression/Generator status of an Operation at compile
  time, and record this information in the Operation. Use nresults_ for that,
  use -1 to indicate a generator. Maybe keep the eval/generate methods.

What does my CPS data structure look like?
* Each Operation class would be changed so that:
  * Strict operands become variable references (the variable is bound in
    an enclosing scope before the Operation is executed).
  * Results are passed as arguments to a continuation (data member), which is:
    * A lambda expression. The parameters have slot numbers in the current
      frame, so at runtime, arguments are passed by stuffing values into
      slots. The body is an Operation.
    * A variable reference to a function value.
      * This could be the 'k' argument to the current function on the call
        stack, in which case we are returning a value from that function.
        Maybe we stuff the return values into the caller's frame, destroy
        the current frame, and jump to the next operation. So, the value k
        consists of a Frame* and an Operation*? Or maybe this is compiled
        into a special 'return' continuation which gets the caller's frame and
        next operation out of the current frame.
      * This could be a regular function value, in which case this is
        a regular tail call. A new frame is constructed for the function call,
        the current frame is destroyed, we jump to the function's head
        Operation.

The Frame contains all of the VM registers. The new interpreter is now
a loop that invokes the current instruction, referenced by the IP.
Each instruction is responsible for advancing the IP, and possibly for
replacing the current Frame with another one. So maybe:
    interpreter(unique_ptr<Frame> f)
    {
        while (f != nullptr)
            f = f->op->exec(std::move(f));
    }
This resembles indirect threading.

So, `unique_ptr<Frame> Operation::exec(unique_ptr<Frame>)`.
The Operation::exec method:
* reads dynamic arguments from the Frame, and static arguments from the
  Operation.
* Performs a computation, usually producing a result, sometimes causing
  side effects.
* "calls its continuation", which means storing result values in the frame,
  updating `f->op` to point to the next Operation, sometimes pushing or popping
  the frame stack (and returning a different frame than it was passed).


f(x) = x + 2;

f = (lambda(x k)
      (const 2 (lambda(r0)
        (add x r0 k))))
