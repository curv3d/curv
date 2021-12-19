// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_FNAME_H
#define LIBCURV_FNAME_H

#include <libcurv/symbol.h>

namespace curv {

// This is the (optional) name of a function.
// It is stored in a Function value, and also in a stack trace.
struct FName {
    // optional name of function
    Symbol_Ref name_{};

    // Suppose this function is the result of partial application of a named
    // function. Then this is the # of arguments that were applied to get here,
    // and `name_` is the name of the base function.
    unsigned argpos_ = 0;

    explicit operator bool() const { return !name_.empty(); }

    friend std::ostream& operator<<(std::ostream& o, const FName& fn) {
        if (fn) {
            o << fn.name_;
            for (unsigned i = 0; i < fn.argpos_; ++i)
                o << " _";
        }
        return o;
    }

    FName(Symbol_Ref nm) : name_(nm) {}
    FName() {}
};

} // namespace curv
#endif // header guard
