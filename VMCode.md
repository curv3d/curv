# VM Code 2017
The 2017 Curv executable format, to replace the 2016 Operation class
and its eval/exec/generate protocol.

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
