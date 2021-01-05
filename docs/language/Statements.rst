Statements
==========
Curv supports imperative style programming by providing statements,
which are executed in sequence for their effects.

The statement language includes mutable local variables,
assignment statements, imperative control structures (sequencing, conditionals,
loops), a print statement (that prints to the debug console), and assertions.

Rationale
---------
Here are some reasons for using the imperative features:

* If you are learning Curv as a modelling language, and you don't wish to
  be a software developer, then imperative ``for`` loops and ``while`` loops
  are simpler and easier to understand than iteration coded in a pure
  functional style using tail recursion. Tail recursion is harder to
  understand; the use of auxiliary functions is inconvenient and adds clutter.
* Assignment statements are the simplest and most intuitive syntax for
  selectively updating elements of a nested data structure.
* Debugging using print statements.
* Embedding assertions and unit tests in your code.
* You are porting graphics algorithms from another language, and you don't
  want the hassle of translating from imperative to functional style.
  Preserving the imperative code structure makes it easier to track
  changes, and easier to reason about performance.
* For functions that execute on the GPU, such as signed distance functions,
  imperative loops are, at present, the only supported form of iteration.
  This is because we are compiling into GPU shader code, which does not
  support recursion. The good news is: with Curv, we can hide this
  imperative code inside a pure functional interface.

Curv provides a deliberately simplified subset of imperative programming.
The goal is to keep the language simple, and by that, I mean Curv has
simple and clear semantics. There is no "spooky action at a distance".
Everything in Curv (source code, data, program execution dynamics) has a
hierarchical structure and compositional semantics. You can understand a thing
using local reasoning, by understanding the meaning of each part in isolation.

Here are some consequences of this design principle:

* Imperative control structures conform to the principles of Structured
  Programming. They have a single entrance and exit. There are no gotos.
* Data is hierarchical. There are no pointers or object references, hence
  there are no cycles.
* Local variables are mutable, but values and data structures are immutable.
  When you update an element of a data structure using an assignment statement,
  it is only the contents of the local variable that changes.
* Functions are pure: the result of a function call depends only on the
  argument values. Function calls do not have side effects.
* Functions can only observe and modify their own mutable local variables.
  There are no mutable global variables. There is no shared mutable state.
* As a corollary, nested closures capture the *values* of local variables,
  they do not capture *references* to local variables.
* In an expression like ``a+b``, the order of evaluation of the operands
  doesn't matter. ``a+b`` is always the same as ``b+a``, no exceptions.
  From within the expression language, you cannot cause or observe side
  effects.

You should think of Curv as a simple language that combines the best parts
of pure functional and imperative programming, while leaving out the parts
that require non-local reasoning and make programs hard to understand.

* These simple semantics are good for Curv users, since Curv is
  meant to be used by designers who are not professional computer programmers.
* These simple semantics are also good for machines. Curv is meant to be
  compiled into optimized machine code that runs on a GPU, so that graphics
  are fast. Compiling a general purpose imperative language like Javascript
  or Python into performant GPU machine code is massively difficult. The
  simple semantics of Curv make it easy to parallelize and supports powerful,
  general optimizations.

Using Statements
----------------
Curv is an expression oriented language: programs and function bodies are
expressions, not statements. You use statements by embedding them in an
expression, such as a ``do`` expression.

``do <statements> in <expression>``
    Execute the statements in sequence, then evaluate the expression and
    return its result. Any top level local variables defined in the statements
    are also visible in the expression.

Curv programs can be debugged using ``print`` statements, ``assert``
statements, etc.
You can attach debug statements to an expression using a ``do`` clause::

    f x =
        do print "calling function f with argument x=$(x)" in
        <some expression>

To write a function in the imperative style, you put all of its logic
inside a ``do`` clause.
This function computes the factorial (product of numbers from 1 to n)::

    factorial n =
        do
            local result = 1;
            local i = 0;
            while (i < n) (
                result := result * i;
                i := i + 1;
            );
        in result

Notes:

* ``local <name> = <value>`` defines a local variable.
* ``<variable> := <value>`` is an assignment statement that modifies
  a previously defined local variable.
* The ``;`` operator is the imperative sequencing operator, which joins
  multiple statements into a compound statement.
  ``a;b`` means execute statement ``a``, then execute statement ``b``.
  A trailing semicolon at the end of a compound statement is ignored.
* The second argument of ``while`` is parenthesized because ``;`` has
  the lowest operator precedence. Parentheses are used here
  for grouping--they are not part of the syntax of a compound statement.

Statement Reference
-------------------
``print message``
    Print a message string on the debug console, followed by newline.
    If ``message`` is not a string, it is converted to a string using
    ``string``. Example::

        print "Entered function f($x):"

    Print statements are part of the debugger; they are a kind of debug
    annotation that doesn't change the meaning or result of the program.

``warning message``
    Print a message string on the debug console, preceded by "WARNING: ",
    followed by newline and then a stack trace.
    If ``message`` is not a string, it is converted to a string using
    ``string``.

``error message``
    On the debug console, print "ERROR: ", then the message string,
    then newline and a stack trace. Then terminate the program.
    If ``message`` is not a string, it is converted to a string using
    ``string``.
    (The name ``error`` is overloaded: it is also the name of a function
    with the same behaviour as the statement.)

``assert condition``
    Evaluate the condition, which must be true or false.
    If it is true, then nothing happens.
    If it is false, then an assertion failure error message is produced,
    followed by a stack trace, and the program is terminated.

``assert_error[error_message_string, expression]``
    Evaluate the expression argument.
    Assert that the expression evaluation terminates with an error,
    and that the resulting error message is equal to ``error_message_string``.
    Used for unit testing.

``exec expression``
    Evaluate the expression and then ignore the result.
    This is used for calling a function whose only purpose is to have a side
    effect (by executing debug statements) and you don't care about the result.

Parenthesized statement: ``(statement)``
    Any statement can be wrapped in parentheses without changing its meaning.
    (Because any *semantically meaningful phrase* can be parenthesized.)
    Example::

        (print "Hello, world.")

Compound statement: ``<statement1>; <statement2>; ...``
    A compound statement is a sequence of statements, separated by
    semicolons, with an optional final semicolon.
    The statements are executed in left-to-right order.
    Example::

        print "Hello"; print "world.";

    You can think of the ``;`` operator as the imperative sequencing operator.
    It is an n-ary operator with 1 or more statements as arguments.

    The ``;`` operator has the lowest possible operator precedence.
    Therefore, a compound statement must be parenthesized when passing
    it as an argument to a control structure like ``if``, ``while``
    or ``for``. The parentheses are for grouping: they
    are not part of the syntax of a compound statement.

Empty statement:
    The empty statement has no tokens, and has no effect.
    An empty statement is parsed when the entire program is empty,
    or when there are no tokens between a pair of parentheses.

    When you hit return in the REPL without typing anything, you are executing
    the empty statement.

    A parenthesized empty statement such as ``()``
    can be passed as an argument to a control structure like
    ``if``, ``while`` or ``for``. You would do this in the same situations
    where you use the empty compound statement ``{}`` in a C-like language.

Local definition: ``local <definition>; <statements>``
    A local definition is an ordinary definition preceded by the keyword
    ``local``. Example::

        local a = 1
        local f x = x + 1
        local include "foo.curv"

    Local definitions may be interleaved with statements in a compound
    statement. The scope of a local variable defined this way begins
    at the statement following the definition and continues to the end
    of the compound statement. Example::

        local x = "world"; print "Hello, $x."
    
    Local definitions use "sequential scoping". Statement order matters:
    a later local definition can refer to variables defined in an earlier
    local definition, but not vice versa. And you can't define recursive
    functions. Use ``let`` for recursively scoped local variables,
    and see `Definitions`_ for definition syntax.

Recursively scoped local variables: ``let <definitions> in <statement>``
    Define local variables over the statement, using recursive scoping.
    The order of definitions doesn't matter. See: `Definitions`_.
    Most imperative languages do not allow you to define recursive
    functions local to a statement block. So this is outside
    of idiomatic imperative programming.

.. _`Definitions`: Definitions.rst

Assignment statement: ``<variable> := <value>``
    An assignment statement modifies a local variable
    defined in an enclosing scope using a ``local`` statement,
    or defined using ``let`` or ``for``.
    Example::

        local msg = "Hello"; msg := msg ++ " world"; print msg;

Conditional statement:
  ``if (condition) statement``
    The statement is only executed if the condition is true.
    See: `Boolean Values`_.

  ``if (condition) statement1 else statement2``
    Execute statement1 if the condition is true, otherwise execute statement2.
    Both statements have the same type.
    See: `Boolean Values`_.

Bounded iteration:
  ``for (pattern in list_expression) statement``
    The statement is executed once for each element in the list.
    At each iteration,
    the element is bound to zero or more local variables by the pattern.
    See: `Patterns`_.

  ``for (pattern in list_expression until condition) statement``
    If you add ``until condition`` to a ``for`` loop,
    then the loop will exit on the first iteration where ``condition`` is true.
    This is how you code early exit from a ``for`` loop:
    there is no ``break`` statement.

Unbounded iteration: ``while (condition) statement``
    The statement is executed repeatedly, zero or more times,
    until ``condition`` becomes false. The condition tests one or
    more local variables which are modified by assignments within
    the loop body on each iteration.

.. _`Boolean Values`: Boolean_Values.rst
.. _`Lists`: Lists.rst
.. _`Records`: Records.rst
.. _`Definitions`: Definitions.rst
.. _`Patterns`: Patterns.rst
