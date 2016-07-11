= Byte Code Format
If I use a byte code interpreter, what's the instruction set and architecture?

Register based is higher performance than stack based, and not any more
difficult to implement. Eg, the Add opcode has 3 immediate operands,
which are stack frame slot indexes.

Opcodes are integers. The main interpreter loop is switch(opcode) inside
of a for(;;). The opcodes are sequential from 0, defined by enum OpCode, and
the default case uses a "notreached" compiler directive to guide optimization:
this is `__assume(false);` on MSC++ and `__builtin_unreachable();` in gcc.
ChakraCore uses this to optimize its interpreter loop; not sure if it helps
on gcc or clang.
* Direct threaded code is a popular alternative.
  In GCC, `goto (*ip++)` is faster than branch to top of loop and switch.
  It takes up more space (64 bits per opcode), so worse cache coherency.
  The `labels as values` gcc extension isn't available in MSC++.
  In 2014, it was alleged that Clang didn't adequately optimize
  this type of interpreter loop: the IP wasn't stored in a register,
  and the code for goto a label value was slower. I dunno if this is fixed,
  but I need clang for MacOS.

An operation consists of a 16 bit opcode, followed by zero or more
16 bit operands. Constants are loaded from a constant table.
Relative branches use a signed 16 bit offset into an array of shorts,
so 32767 ops could be an upper limit, unless I provide an extended branch
with a 32 bit operand for those special cases.
This seems good and simple: no memory alignment issues, and I won't run
out of registers (8 bit slot ids are limiting).

Rationale:
* Compact is good, fewer cache hits, more cache coherence.
* Opcodes can be 1 byte, I won't likely need more than 256 opcodes.
  But immediate operands need to be more than 1 byte,
  and that raises the issue of unaligned memory access.
  * Eg, relative branch.
  * I might need stack frames with more than 256 value slots?
  * Eg, immediate 8 byte values?
* Intel and ARM support unaligned memory access. ARMv7 needs to use
  special unaligned instructions: LDRD vs LDR. The compiler needs to know.
  MSC++ has an `unaligned` pointer type decoration, g++ doesn't
  (the documentation for `__attribute__(aligned(1))` doesn't give hope).
  If I have to use memcpy() to fix an alignment problem in the interpreter
  main loop, my performance will be crap.
* The clang mailing list describes using
  `typedef int32_t unaligned_int32_t __attribute__((aligned(1)));`
  to make unaligned pointers safe to use. So maybe I'm good?
  The same post gives reasons not to rely on this: the attribute isn't
  part of the type system, could be dropped in various contexts like template
  parameters and ?: operator.
* I could use 'short codes' instead of 'byte codes'.
  An instruction array is an array of uint16_t. Immediate operands are 16 bit.
  Constants are loaded from a constant table.
  A few operations might need rarely used variants with 32 bit
  immediate operands (eg, branch and load constant). I won't support stack
  frames with more than 65536 value slots.
* This seems good and simple. No alignment issues. I won't run out of registers,
  and I won't need to consider 8 and 16 bit versions of every arithmetic opcode.

Problem: I need 4 operands for Add: the 4th is a source pointer
`(Phrase*)` for reporting an error. This is kind of like how python and ruby
apparently have line numbers embedded in their operations.
* I don't really want to embed a 64 bit pointer in the instruction stream
  that is almost never used: poor cache locality.
* I could have a location table (array of `Phrase*`) and embed a 16 bit
  location index into the instruction. 25% more storage, but better
  cache locality. Easy to implement.
* When emitting an Add operation, instead of putting the location into the
  location table and emitting the location index inline, I could instead
  add an (opcode index, location) pair to an external table. Then, when an
  error occurs, use the opcode index to look up the location. Not intrinsically
  more difficult, improves cache locality, and the external table could later
  be extended or rearranged to support a reverse map from source to
  instructions.
* The opcode index could be stored in the Phrase. Then we exhaustively search
  the source tree for the index when an error occurs. This would speed up
  compile and slow down error handling, which is fine.
* I could have a location map that maps opcode ranges onto source pointers.
  The opcode array is split into a sequence of contiguous ranges.
  For each range, we store the length of the range (16 bits)
  and a source pointer. We use two arrays to eliminate alignment gaps.
  More work. Could be used to set a breakpoint at a source location by
  pointing to code in the editor? Single stepping?


It's unlikely I need more than 256 opcodes, which will fit in a byte.
What about literal operands, and their alignment?
A curv::Value is 8 bytes. I'll also need offsets or pointers for jumps,
and offsets relative to frame pointers for loading variables.
I could use 32 bit opcodes with an 8 bit operator field and 24 bits of
immediate operand. Constant Values are stored in a literal table.
* That's a lot of empty operand fields. Poor cache coherence.
  Ela uses two arrays of the same size, an ops array and an int32 opData array.
* Lua uses 32 bit opcodes with a 6 bit operation; register machine.
* CPython 3.3:
  * 101 opcodes: stack, flow ctrl, arithmetic, constructors, details.
  * 1 byte operation, optional unaligned 2 byte argument may follow
  * registers: IP, stack, fast_locals (local variables and parameters)
  * load_global is an index into an array of names
* Ruby

**Function Calls.**
Several cases:
* **Call to an unknown dynamic function value.**
  Here we are computing a function value at run time, which is stored in a
  value slot. With the current simplified function design, we compare the
  provided # of arguments with the nargs value in the function value, and
  abort if they are different. So: push the arguments,
  then `opCall(funSlot,nargs)`. Abort if fun->nargs != nargs,
  otherwise push the return value and branch to the function.
* **Call to a known static function.** In the machine code interpreter, this
  would be optimized in the same way as in C, with a call to a static address.
  Also, nargs checking happens at compile time.
  In the bytecode interpreter, the function is in the constant table,
  so `opCallConstant(funSlot)`. [builtin functions are accessed via the
  constant table, apparently.]
* ...

I could abstract the difference between different bytecode representations
using an abstract interface for generating and executing code.

I will separate the control stack from the argument stack.
One is pointers and one is doubles: potentially different alignment.

Registers:
* SP: stack pointer (value stack)
* FP: frame pointer (value stack)
* KP: constants
* IP: instruction pointer
* CS: control stack pointer

Function call:
* validate length of argument list (compile or run time)
* push arguments. SP points at 1+the last argument.
* call function. CS points at 1+the return address. IP points to function body.
* return: remember the top of stack value; pop all temporaries and the original
  arguments; push the return value; IP = pop the return address.
