// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GL_COMPILER_H
#define LIBCURV_GL_COMPILER_H

#include <unordered_map>
#include <vector>
#include <libcurv/gl_frame.h>
#include <libcurv/meaning.h>

namespace curv {

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

enum class GL_Target
{
    glsl,   // output GLSL code
    cpp     // output C++ code using GLM library
};

struct Op_Hash
{
    size_t operator()(Shared<const Operation> op) const noexcept
    {
        return op->hash();
    }
};
struct Op_Hash_Eq
{
    bool operator()(
        Shared<const Operation> op1, Shared<const Operation> op2)
        const noexcept
    {
        return op1->hash_eq(*op2);
    }
};

/// Global state for the GLSL code generator.
struct GL_Compiler
{
    std::ostream& out_;
    GL_Target target;
    unsigned valcount_;
    System &system_;
    std::unordered_map<Value, GL_Value, Value::Hash, Value::Hash_Eq>
        valcache_{};
    std::unordered_map<Shared<const Operation>, GL_Value, Op_Hash, Op_Hash_Eq>
        opcache_{};

    GL_Compiler(std::ostream& s, GL_Target t, System& sys)
    :
        out_(s), target(t), valcount_(0), system_(sys)
    {}

    std::ostream& out() { return out_; }

    inline GL_Value newvalue(GL_Type type)
    {
        return GL_Value(valcount_++, type);
    }

    inline void newfunction()
    {
        valcount_ = 0;
        valcache_.clear();
        opcache_.clear();
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

GL_Value gl_eval_op(GL_Frame& f, const Operation& op);
GL_Value gl_eval_expr(GL_Frame&, const Operation& op, GL_Type);
GL_Value gl_eval_const(GL_Frame& f, Value val, const Phrase&);
GL_Value gl_call_unary_numeric(GL_Frame&, const char*);
void gl_put_as(GL_Frame& f, GL_Value val, const Context&, GL_Type type);
GL_Value gl_vec_element(GL_Frame&, GL_Value, int);

} // namespace
#endif // header guard
