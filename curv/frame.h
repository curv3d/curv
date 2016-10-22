// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_FRAME_H
#define CURV_FRAME_H

#include <aux/tail_array.h>
#include <curv/value.h>
#include <curv/list.h>

namespace curv {

struct Frame_Base
{
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

    Frame_Base(List* nl) : nonlocal(nl) {}
};

/// A Frame is an evaluation context.
///
/// You can think of a Frame as containing all of the registers used
/// by the Curv virtual machine.
///
/// Each top-level module has a frame for evaluating subexpressions
/// while constructing the module value.
/// Builtin and user-defined functions have call frames.
using Frame = aux::Tail_Array<Frame_Base>;

} // namespace curv
#endif // header guard
