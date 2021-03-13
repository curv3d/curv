// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_FUNC_LOC_H
#define LIBCURV_FUNC_LOC_H

#include <libcurv/location.h>
#include <libcurv/function.h>

namespace curv {

// A Func_Loc is location data used to print an element of a stack trace.
struct Func_Loc
{
    Func_Loc(Src_Loc srcloc)
    :
        func_(nullptr), srcloc_(srcloc)
    {}
    Func_Loc(Shared<const Function> func, Src_Loc srcloc)
    :
        func_(func), srcloc_(srcloc)
    {}
    // Function whose definition lexically encloses the srcloc, or null.
    // This function is determined dynamically (taken from the stack frame).
    // The thing we want from the function is its name (aka its metadata).
    // In theory, we could have statically associated the name of the enclosing
    // function to the srcloc at parse or analysis time. But that's inadequate
    // if there are several aliases referring to the same lexical function
    // definition. In that case, we want dynamic information, so we can report
    // the function name used in the function call.
    Shared<const Function> func_;
    // Source code location of a phrase that caused a run time panic, or
    // that was a function call on the stack at the time of the panic.
    // It may be a function call, in which case, the function is `func_`.
    Src_Loc srcloc_;

    // Output a stack trace element, which is printed as part of
    // an exception message (no final newline). See Exception::write().
    // The `colour` flag enables colour text using ANSI ESC sequences.
    // The `many` flag is true if the stack trace has more than one element.
    void write(std::ostream&, bool colour, bool many) const;

    void write_json(std::ostream&) const;
};

} // namespace curv
#endif // header guard
