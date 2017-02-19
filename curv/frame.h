// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_FRAME_H
#define CURV_FRAME_H

#include <aux/tail_array.h>
#include <curv/list.h>
#include <curv/value.h>

namespace curv {

class Frame_Base;
class Call_Phrase;
class System;

/// A Frame is an evaluation context.
///
/// You can think of a Frame as containing all of the registers used
/// by the Curv virtual machine.
///
/// Each top-level module has a frame for evaluating subexpressions
/// while constructing the module value.
/// Builtin and user-defined functions have call frames.
using Frame = aux::Tail_Array<Frame_Base>;

struct Frame_Base
{
    /// The System object abstracts client- and os-specific functionality.
    /// It is owned by the client, and is generally available to the evaluator.
    /// A reference to the global System object is stored in every Frame,
    /// because that seems more efficient (less copying) than passing it as
    /// a parameter to every `eval` call, and it seems cleaner than a thread
    /// local variable. Think of the System reference as a VM register.
    System& system;

    /// Frames are linked into a stack. This is metadata used for printing
    /// a stack trace and by the debugger. It is not used during evaluation.
    Frame* parent_frame;

    /// If this is a function call frame, then call_phrase is the source code
    /// for the function call, otherwise it's nullptr. This is debug metadata.
    /// Module frames do not have a call_phrase. If the call_phrase is null,
    /// then the frame does not appear in a stack trace.
    const Call_Phrase* call_phrase;

    /// Slot array containing the values of nonlocal bindings.
    ///
    /// This is:
    /// * the slot array of Module value, for a top level module frame
    /// * the slot array of a Closure value, for a function call frame
    /// * nullptr, for a call to a builtin function.
    List* nonlocal;

    // Tail array, containing the slots used for local bindings:
    // function arguments, `let` bindings and other local, temporary values.
    using value_type = Value;
    size_t size_;
    value_type array_[0];

    Value& operator[](size_t i)
    {
        assert(i < size_);
        return array_[i];
    }

    Frame_Base(System& sys, Frame* parent, const Call_Phrase* src, List* nl)
    :
        system(sys),
        parent_frame(parent),
        call_phrase(src),
        nonlocal(nl)
    {}
};

} // namespace curv
#endif // header guard
