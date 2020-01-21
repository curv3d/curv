// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SC_COMPILER_H
#define LIBCURV_SC_COMPILER_H

#include <unordered_map>
#include <vector>
#include <libcurv/sc_frame.h>
#include <libcurv/meaning.h>

namespace curv {

struct Context;
struct Function;
struct System;

/// SubCurv is a low level, strongly-typed subset of Curv
/// that can be efficiently translated into a low level language (currently
/// C++ or GLSL), for fast evaluation on a CPU or GPU.
///
/// SubCurv is a set of types and operations on those types. It is a statically
/// typed subset of Curv which is also a subset of GLSL (but with different
/// type and operation names). Curv distance functions must be restricted to the
/// SC subset or the Shape Compiler will report an error during rendering.
///
/// The compiler operates on Curv function *values*. Non-local variables
/// captured by closures become compile time constants. Intermediate function
/// calls are inline expanded: this eliminates first-class function values,
/// which aren't supported by GLSL, and it eliminates run-time polymorphism,
/// where different calls to the same function have different argument types.
/// The generated code is statically typed, and uses SSA style, where each
/// operation is represented by an assignment to an SSA variable.

enum class SC_Target
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
using Op_Cache =
    std::unordered_map<Shared<const Operation>, SC_Value, Op_Hash, Op_Hash_Eq>;

/// Global state for the GLSL/C++ code generator.
struct SC_Compiler
{
    std::ostream& out_;
    std::stringstream constants_{};
    std::stringstream body_{};
    bool in_constants_ = false;
    SC_Target target_;
    unsigned valcount_;
    System &system_;
    std::unordered_map<Value, SC_Value, Value::Hash, Value::Hash_Eq>
        valcache_{};
    std::vector<Op_Cache> opcaches_{};

    SC_Compiler(std::ostream& s, SC_Target t, System& sys)
    :
        out_(s), target_(t), valcount_(0), system_(sys)
    {
    }

    std::ostream& out()
    {
        if (in_constants_)
            return constants_;
        else
            return body_;
    }

    // This is the main entry point to the Shape Compiler.
    void define_function(
        const char* name, SC_Type param_type, SC_Type result_type,
        Shared<const Function> func, const Context&);
    void define_function(
        const char* name,
        std::vector<SC_Type> param_types,
        SC_Type result_type,
        Shared<const Function> func,
        const Context& cx);

    void begin_function();
    void end_function();

    inline SC_Value newvalue(SC_Type type)
    {
        return SC_Value(valcount_++, type);
    }

    // TODO: maybe add a member function for each operation that we support.
    // Maybe these can later be virtual functions, so that this interface
    // becomes generic for SPIR-V and LLVM code generation. Eg,
    // SC_Value add(SC_Value x, SC_Value y)
    // {
    //     assert(x.type == SC_Type::num);
    //     assert(y.type == SC_Type::num);
    //     auto result = newvalue(SC_Type::num);
    //     out << result << "=" << x << "+" << y << ";\n";
    //     return result;
    // }
    //
    // This API looks handy for code generation for built-in shapes
    // with built-in dist functions. You can nest these function calls,
    // it would look Lispy. sc.sqrt(sc.add(sc.square(x), sc.square(y)))
    //
    // The caller is responsible for passing arguments (to `add`) of the
    // correct type. (Otherwise, there is an assertion failure.)
    // Only the caller has enough context to arrange for expressive exceptions
    // to report a shape argument of the wrong type, or a failure of static
    // type checking within a user-defined dist function.
    //
    // Maybe SC_Compiler will track context for expressive exceptions?
    // There is no eval stack at this time, but there is a CSG tree,
    // so maybe we can have a CSG tree stack trace. There is also a
    // sc_call stack, once we support nested function calls in dist functions.
};

SC_Value sc_eval_op(SC_Frame& f, const Operation& op);
SC_Value sc_eval_expr(SC_Frame&, const Operation& op, SC_Type);
SC_Value sc_eval_num_or_vec(SC_Frame&, const Operation& op);
SC_Value sc_eval_const(SC_Frame& f, Value val, const Phrase&);
SC_Value sc_call_unary_numeric(SC_Frame&, const char*);
void sc_put_as(SC_Frame& f, SC_Value val, const Context&, SC_Type type);
SC_Value sc_vec_element(SC_Frame&, SC_Value, int);
void sc_conform_numeric(
    SC_Frame& f, SC_Value& x, SC_Value& y, const Context& cx);

} // namespace
#endif // header guard
