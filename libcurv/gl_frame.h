// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GL_FRAME_H
#define LIBCURV_GL_FRAME_H

#include <ostream>
#include <libcurv/gl_type.h>
#include <libcurv/module.h>
#include <libcurv/tail_array.h>

namespace curv {

struct Context;
struct GL_Compiler;
struct Phrase;

/// An SSA variable used during GL code generation.
struct GL_Value
{
    unsigned index;
    GL_Type type;

    GL_Value(unsigned i, GL_Type t) : index(i), type(t) {}
    GL_Value() noexcept {}
};

/// print the GLSL variable name
inline std::ostream& operator<<(std::ostream& out, GL_Value v)
{
    out << "r" << v.index;
    return out;
}

struct GL_Frame_Base;
using GL_Frame = Tail_Array<GL_Frame_Base>;

/// A function call frame used by the Geometry Compiler.
///
/// The GL compilation process is a kind of abstract evaluation.
/// That's really clear when you see that GL_Frame is isomorphic to Frame,
/// with local slot Values replaced by GL_Values.
struct GL_Frame_Base
{
    GL_Compiler& gl;

    /// The root frame has a context pointer, which points to the shape
    /// expression that is being compiled. Used for printing a stack trace.
    const Context* root_context_;

    /// Frames are linked into a stack. This is metadata used for printing
    /// a stack trace and by the debugger. It is not used during evaluation.
    GL_Frame* parent_frame_;

    /// If this is a function call frame, then call_phrase_ is the source code
    /// for the function call. If it is a frame for a reactive value expression,
    /// then it's source code for the expression. Otherwise it's nullptr.
    /// If not null, then the phrase creates a stack trace entry.
    const Phrase* call_phrase_;

    /// Slot array containing the values of nonlocal bindings.
    ///
    /// This is:
    /// * the slot array of a Closure value, for a function call frame
    /// * nullptr, for a call to a builtin function.
    Module* nonlocals_;

    // Tail array, containing the slots used for local bindings:
    // function arguments, block bindings and other local, temporary values.
    using value_type = GL_Value;
    size_t size_;
    value_type array_[0];

    GL_Value& operator[](size_t i)
    {
        assert(i < size_);
        return array_[i];
    }

    GL_Frame_Base(
        GL_Compiler& g,
        const Context* cx,
        GL_Frame* parent,
        const Phrase* src)
    :
        gl(g),
        root_context_(cx),
        parent_frame_(parent),
        call_phrase_(src),
        nonlocals_(nullptr)
    {}
};

} // namespace
#endif // header guard
