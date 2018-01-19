// Copyright Doug Moen 2016-2018.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_FRAME_H
#define CURV_FRAME_H

#include <curv/tail_array.h>
#include <curv/list.h>
#include <curv/value.h>
#include <curv/slot.h>
#include <curv/module.h>

namespace curv {

struct Frame_Base;
struct Call_Phrase;
struct System;

/// A Frame is an evaluation context.
///
/// You can think of a Frame as containing all of the registers used
/// by the Curv virtual machine.
///
/// A program (script file) has a frame for evaluating the top level
/// program expression.
/// Calls to builtin and user-defined functions have call frames.
using Frame = Tail_Array<Frame_Base>;

struct Frame_Base
{
    /// The System object abstracts client- and os-specific functionality.
    /// It is owned by the client, and is generally available to the evaluator.
    /// A reference to the global System object is stored in every Frame,
    /// because that seems more efficient (less copying) than passing it as
    /// a parameter to every `eval` call, and it seems cleaner than a thread
    /// local variable. Think of the System reference as a VM register.
    System& system_;

    /// Frames are linked into a stack. This is metadata used for printing
    /// a stack trace and by the debugger. It is not used during evaluation.
    Frame* parent_frame_;

    /// If this is a function call frame, then call_phrase_ is the source code
    /// for the function call, otherwise it's nullptr. This is debug metadata.
    /// Program frames do not have a call_phrase_. If the call_phrase_ is null,
    /// then the frame does not appear in a stack trace.
    const Call_Phrase* call_phrase_;

    /// Slot array containing the values of nonlocal bindings.
    ///
    /// This is:
    /// * the slot array of a Closure value, for a function call frame.
    /// * nullptr, for a builtin function call, or a program frame.
    Module* nonlocals_;

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

    Frame_Base(System& sys, Frame* parent, const Call_Phrase* src, Module* nl)
    :
        system_(sys),
        parent_frame_(parent),
        call_phrase_(src),
        nonlocals_(nl)
    {}
};

} // namespace curv
#endif // header guard
