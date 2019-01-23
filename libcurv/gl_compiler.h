// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GL_COMPILER_H
#define LIBCURV_GL_COMPILER_H

#include <ostream>
#include <unordered_map>
#include <vector>
#include <libcurv/tail_array.h>
#include <libcurv/module.h>
#include <libcurv/gl_type.h>

namespace curv {

struct Operation;
struct Phrase;
struct List_Expr_Base;
using List_Expr = Tail_Array<List_Expr_Base>;
struct Lambda_Expr;
struct Context;
struct System;

/// "GL" or "Geometry Language" is a low level, strongly-typed subset of Curv
/// that can be efficiently translated into a low level language (currently
/// C++ or GLSL), for fast evaluation on a CPU or GPU.
///
/// GL is a set of types and operations on those types. It is a statically
/// typed subset of Curv which is also a subset of GLSL (but with different
/// type and operation names). Curv distance functions must be restricted to the
/// GL subset or the Geometry Compiler will report an error during rendering.
///
/// The compiler operates on Curv function *values*. Non-local variables
/// captured by closures become compile time constants. Intermediate function
/// calls are inline expanded: this eliminates first-class function values,
/// which aren't supported by GLSL, and it eliminates run-time polymorphism,
/// where different calls to the same function have different argument types.
/// The generated code is statically typed, and uses SSA style, where each
/// operation is represented by an assignment to an SSA variable.

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

enum class GL_Target
{
    glsl,   // output GLSL code
    cpp     // output C++ code using GLM library
};

/// Global state for the GLSL code generator.
struct GL_Compiler
{
    std::ostream& out;
    GL_Target target;
    unsigned valcount_;
    System &system_;
    std::unordered_map<Value, GL_Value, Value::Hash, Value::Hash_Eq> valcache_{};

    GL_Compiler(std::ostream& s, GL_Target t, System& sys)
    :
        out(s), target(t), valcount_(0), system_(sys)
    {}

    inline GL_Value newvalue(GL_Type type)
    {
        return GL_Value(valcount_++, type);
    }

    inline void newfunction()
    {
        valcount_ = 0;
        valcache_.clear();
    }

    // TODO: maybe add a member function for each operation that we support.
    // Maybe these can later be virtual functions, so that this interface
    // becomes generic for SPIR-V and LLVM code generation. Eg,
    // GL_Value add(GL_Value x, GL_Value y)
    // {
    //     assert(x.type == GL_Type::num);
    //     assert(y.type == GL_Type::num);
    //     auto result = newvalue(GL_Type::num);
    //     out << result << "=" << x << "+" << y << ";\n";
    //     return result;
    // }
    //
    // This API looks handy for code generation for built-in shapes
    // with built-in dist functions. You can nest these function calls,
    // it would look Lispy. gl.sqrt(gl.add(gl.square(x), gl.square(y)))
    //
    // The caller is responsible for passing arguments (to `add`) of the
    // correct type. (Otherwise, there is an assertion failure.)
    // Only the caller has enough context to arrange for expressive exceptions
    // to report a shape argument of the wrong type, or a failure of static
    // type checking within a user-defined dist function.
    //
    // Maybe GL_Compiler will track context for expressive exceptions?
    // There is no eval stack at this time, but there is a CSG tree,
    // so maybe we can have a CSG tree stack trace. There is also a
    // gl_call stack, once we support nested function calls in dist functions.
};

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

GL_Value gl_eval_expr(GL_Frame&, const Operation& op, GL_Type);
GL_Value gl_eval_const(GL_Frame& f, Value val, const Phrase&);
GL_Value gl_call_unary_numeric(GL_Frame&, const char*);
void gl_put_as(GL_Frame& f, GL_Value val, const Context&, GL_Type type);
GL_Value gl_vec_element(GL_Frame&, GL_Value, int);

} // namespace
#endif // header guard
