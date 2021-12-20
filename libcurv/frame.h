// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_FRAME_H
#define LIBCURV_FRAME_H

#include <libcurv/tail_array.h>
#include <libcurv/value.h>
#include <libcurv/slot.h>
#include <libcurv/module.h>

namespace curv {

struct Context;
struct Frame_Base;
struct Function;
struct Operation;
struct Phrase;
struct String_Ref;
struct System;
struct Source_State;

/// A Frame is an evaluation context.
///
/// You can think of a Frame as containing all of the registers used
/// by the Curv virtual machine.
///
/// A program (source file) has a frame for evaluating the top level
/// program expression.
/// Calls to builtin and user-defined functions have call frames.
struct Frame;
struct Frame_Base
{
    // sstate_ contains shared state for processing the current source file,
    // and provides handles to global state.
    // A reference is stored in every Frame: consider this a VM register.
    Source_State& sstate_;

    /// Frames are linked into a stack. This is metadata used for printing
    /// a stack trace and by the debugger. It is not used during evaluation.
    Frame* parent_frame_;

    /// Slot array containing the values of nonlocal bindings.
    ///
    /// This is:
    /// * the slot array of a Closure value, for a function call frame.
    /// * nullptr, for a builtin function call, or a program frame.
    Module* nonlocals_;

    // Registers used by tail_eval_frame and Operation::tail_eval.
    // next_op_ is the next Operation to be executed by the tail-evaluation
    // interpreter loop, or it is nullptr, in which case result_ holds the
    // evaluation result.
    const Operation* next_op_;
    Value result_;

    // If this is a function call frame, then `func_` is the function that
    // was called. Otherwise, nullptr. Used to initialize caller_.
    // Used to print stack traces (At_Phrase only).
    // When calling a Closure, a counted reference to `func_` keeps
    // the 'nonlocals_' and 'next_op_' objects alive.
    Shared<const Function> func_;

    // If this is a function call frame (call_phrase_ != nullptr), then caller_
    // is a function whose definition lexically encloses the call_phrase_.
    // Otherwise it is nullptr. Used to print stack traces.
    Shared<const Function> caller_;

    /// If this is a function call frame, then call_phrase_ is the source code
    /// for the function call, otherwise it's nullptr.
    ///
    /// Program frames do not have a call_phrase_. If the call_phrase_ is null,
    /// then the frame does not appear in a stack trace.
    ///
    /// In the common case, *call_phrase_ is a Call_Phrase. However, in the
    /// case of a builtin function B that takes a function F as an argument,
    /// there is no Call_Phrase in Curv source code where F is called, so
    /// Call_Phrase is a best effort approximation, such as the call to B.
    Shared<const Phrase> call_phrase_;

    // Tail array, containing the slots used for local bindings:
    // function arguments, block bindings and other local, temporary values.
    using value_type = Value;
    slot_t size_;
    value_type array_[0];

    Value& operator[](slot_t i)
    {
        assert(i < size_);
        return array_[i];
    }

    Frame_Base(Source_State&, Frame* parent,
        Shared<const Function> caller, Shared<const Phrase> call_phrase);
};
struct Frame final : public Tail_Array<Frame_Base>
{
    using Tail_Array<Frame_Base>::Tail_Array;
};

Value tail_eval_frame(std::unique_ptr<Frame>);

} // namespace curv
#endif // header guard
