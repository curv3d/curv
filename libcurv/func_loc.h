// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_FUNC_LOC_H
#define LIBCURV_FUNC_LOC_H

#include <libcurv/location.h>
#include <libcurv/function.h>

namespace curv {

// A Func_Loc is location data used in printing an element of a stack trace.
struct Func_Loc
{
    Func_Loc(Src_Loc srcloc)
    :
        srcloc_(srcloc)
    {}
  #if 0
    Func_Loc(Symbol_Ref funcname, Src_Loc srcloc)
    :
        funcname_(funcname), srcloc_(srcloc)
    {}
  #else
    Func_Loc(Shared<const Function> func, Src_Loc srcloc)
    :
        func_(func), srcloc_(srcloc)
    {}
  #endif
    // Name of a function whose definition lexically encloses the srcloc,
    // or null. This function name is determined dynamically: in case there
    // are several aliases referring to the same lexical function definition,
    // we use the function name used in the call.
    //Symbol_Ref funcname_ = {};
    Shared<const Function> func_ = nullptr;
    // Source code location of a phrase that caused a run time panic, or
    // that was a function call on the stack at the time of the error.
    // It may be a function call, in which case, if the function has a name,
    // then that name is in `funcname_`.
    Src_Loc srcloc_;

    /// Output the location part of a stack trace element, which is
    /// printed as part of an exception message (no final newline).
    /// The `colour` flag enables colour text using ANSI ESC sequences.
    /// The `many` flag is true if the stack trace has more than one element.
    void write(std::ostream&, bool colour, bool many) const;

    void write_json(std::ostream&) const;
};

} // namespace curv
#endif // header guard
