// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_PROGRAM_H
#define LIBCURV_PROGRAM_H

#include <libcurv/analyser.h>
#include <libcurv/frame.h>
#include <libcurv/meaning.h>
#include <libcurv/module.h>
#include <libcurv/scanner.h>
#include <libcurv/source.h>
#include <libcurv/system.h>

namespace curv {

// A Program is an object that is used to compile and run a Curv program.
// Since libcurv has no global variables, the 'global' state used to compile
// and run a program is created and managed by Program.
// After the program has been evaluated to a Value, the Program retains the
// program's source code and execution context, so that this context can be
// used to report a bad value error, via the At_Program context type.
//
// A Program progresses through 4 phases.
//  1. Construction. Constructs `sstate_`, the Source_State object,
//     containing global variables for compilation and runtime.
//  2. Compilation. Call one of the `compile()` methods to compile the
//     program's source code. Initializes `phrase_`, containing the parse
//     tree of the source code. Initializes internal members that
//     represent the executable form of the program.
//  3. Run the program by calling `eval()` or `exec()`.
//  4. After calling eval() to get a value, use At_Program(prog) as an
//     Exception context to report a bad value.
struct Program
{
    Source_State sstate_;               // initialized by constructor
    Shared<Phrase> phrase_ = nullptr;   // initialized by compile()
private:
    // compiled program representation, initialized by compile()
    Shared<Meaning> meaning_ = nullptr;
    Shared<Module_Expr> module_ = nullptr;
    std::unique_ptr<Frame> frame_ = nullptr;

public:
    /*
     * 1. construct the Program
     */
    Program(System& sys, Frame* file_frame = nullptr)
    :
        sstate_(sys, file_frame)
    {}

    /*
     * 2. compile the Program
     */
    // This is the general interface for compiling source code.
    // Several special-case shortcuts are also provided.
    void compile(Shared<const Source>, Scanner_Opts, Environ&, Interp);

    // This shortcut leaves out Scanner_Opts,
    // which only one specialized client uses.
    void compile(Shared<const Source>, Environ&, Interp);

    // Compile an expression using the std_namespace. The common case.
    void compile(Shared<const Source>);

    // Compile an expression using the std_namespace and a Scanner_Opts.
    void compile(Shared<const Source>, Scanner_Opts);

    /*
     * 3. run the compiled Program
     */
    Shared<Module> exec(Operation::Executor&);
    Value eval();

    /*
     * 4. use the Program to report errors
     */
    const Phrase& syntax() const;
};

} // namespace curv
#endif // header guard
