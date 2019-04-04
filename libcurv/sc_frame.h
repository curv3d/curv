// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SC_FRAME_H
#define LIBCURV_SC_FRAME_H

#include <ostream>
#include <libcurv/sc_type.h>
#include <libcurv/module.h>
#include <libcurv/tail_array.h>

namespace curv {

struct Context;
struct SC_Compiler;
struct Phrase;

/// An SSA variable used during SC code generation.
struct SC_Value
{
    unsigned index;
    SC_Type type;

    SC_Value(unsigned i, SC_Type t) : index(i), type(t) {}
    SC_Value() noexcept {}
};

/// print the GLSL variable name
inline std::ostream& operator<<(std::ostream& out, SC_Value v)
{
    out << "r" << v.index;
    return out;
}

struct SC_Frame_Base;
using SC_Frame = Tail_Array<SC_Frame_Base>;

/// A function call frame used by the Shape Compiler.
///
/// The SC compilation process is a kind of abstract evaluation.
/// That's really clear when you see that SC_Frame is isomorphic to Frame,
/// with local slot Values replaced by SC_Values.
struct SC_Frame_Base
{
    SC_Compiler& sc_;

    /// The root frame has a context pointer, which points to the shape
    /// expression that is being compiled. Used for printing a stack trace.
    const Context* root_context_;

    /// Frames are linked into a stack. This is metadata used for printing
    /// a stack trace and by the debugger. It is not used during evaluation.
    SC_Frame* parent_frame_;

    /// If this is a function call frame, then call_phrase_ is the source code
    /// for the function call. If it is a frame for a reactive value expression,
    /// then it's source code for the expression. Otherwise it's nullptr.
    /// If not null, then the phrase creates a stack trace entry.
    Shared<const Phrase> call_phrase_;

    /// Slot array containing the values of nonlocal bindings.
    ///
    /// This is:
    /// * the slot array of a Closure value, for a function call frame
    /// * nullptr, for a call to a builtin function.
    Module* nonlocals_;

    // Tail array, containing the slots used for local bindings:
    // function arguments, block bindings and other local, temporary values.
    using value_type = SC_Value;
    size_t size_;
    value_type array_[0];

    SC_Value& operator[](size_t i)
    {
        assert(i < size_);
        return array_[i];
    }

    SC_Frame_Base(
        SC_Compiler& sc,
        const Context* cx,
        SC_Frame* parent,
        Shared<const Phrase> src)
    :
        sc_(sc),
        root_context_(cx),
        parent_frame_(parent),
        call_phrase_(std::move(src)),
        nonlocals_(nullptr)
    {}
};

} // namespace
#endif // header guard
