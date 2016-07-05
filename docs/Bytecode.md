= Byte Code Format
If I use a byte code interpreter, what's the instruction set and architecture?

It's stack based. Push evaluated arguments onto stack, invoke operation.
Operations with special syntax get an opcode, others are functions.

x + y => push x; push y; add

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
