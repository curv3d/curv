// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_FRAME_H
#define CURV_FRAME_H

#include <aux/tail_array.h>
#include <curv/value.h>

namespace curv {

class Module;

struct Frame_Base
{
    // top level module value
    Module& module_;

    // tail array, containing the slots used for function arguments,
    // `let` bindings and other local, temporary values.
    using value_type = Value;
    size_t size_;
    value_type array_[0];

    Value& operator[](size_t i)
    {
        assert(i < size_);
        return array_[i];
    }

    Frame_Base(Module& m) : module_(m) {}
};

/// A Frame is an evaluation context.
/// Currently, each top-level module has a frame for evaluating subexpressions.
/// In the future, user-defined functions will have call frames.
/// TODO: Future design of Frame:
/// * Value* nonlocal;
/// * Value local[0]; // tail array
using Frame = aux::Tail_Array<Frame_Base>;

} // namespace curv
#endif // header guard
