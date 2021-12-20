// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SSTATE_H
#define LIBCURV_SSTATE_H

namespace curv {

struct Context;
struct Frame;
struct System;

// Shared state while scanning/analysing/evaluating a source file.
// Referenced by Scanner, Environ (analysis) and Frame (evaluation).
struct Source_State
{
    System& system_;

    // If file_frame_ != nullptr, then we are processing a source file due to
    // an evaluation-time call to `file`, and this is the Frame of the `file`
    // call. It's used to add a stack trace to analysis time errors.
    Frame* file_frame_;

    // Have we already emitted a 'deprecated' warning for this topic?
    // Used to prevent an avalanche of warning messages.
    bool var_deprecated_ = false;
    bool paren_empty_list_deprecated_ = false;
    bool paren_list_deprecated_ = false;
    bool not_deprecated_ = false;
    bool dot_string_deprecated_ = false;
    bool string_colon_deprecated_ = false;
    bool where_deprecated_ = false;
    bool bracket_index_deprecated_ = false;
    bool at_deprecated_ = false;

    Source_State(System& sys, Frame* ff) : system_(sys), file_frame_(ff) {}

    void deprecate(bool Source_State::*, int, const Context&, String_Ref);
    static const char dot_string_deprecated_msg[];
};

} // namespace curv
#endif // header guard
