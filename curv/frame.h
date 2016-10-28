// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_FRAME_H
#define CURV_FRAME_H

#include <list>
#include <aux/tail_array.h>
#include <curv/value.h>
#include <curv/list.h>

namespace curv {

class Frame_Base;
class Phrase;

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
    /// Frames are linked into a stack. This is metadata used for printing
    /// a stack trace and by the debugger. It is not used during evaluation.
    Frame* parent_frame;

    /// If this is a function call frame, then call_phrase is the source code
    /// for the function call, otherwise it's nullptr. This is debug metadata.
    const Phrase* call_phrase;

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

    Frame_Base(Frame* parent, const Phrase* src, List* nl)
    :
        parent_frame(parent),
        call_phrase(src),
        nonlocal(nl)
    {}

    static void get_locations(const Frame* f, std::list<Location>& locs)
    {
        for (; f != nullptr; f = f->parent_frame)
            if (f->call_phrase != nullptr)
                locs.push_back(f->call_phrase->location());
    }
};

} // namespace curv
#endif // header guard
