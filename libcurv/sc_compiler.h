// Copyright 2016-2020 Doug Moen
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

/// SubCurv is a low level, strongly-typed subset of Curv
/// that can be efficiently translated into a low level language (currently
/// C++ or GLSL), for fast evaluation on a CPU or GPU.
///
/// SubCurv is a set of types and operations on those types. It is a statically
/// typed subset of Curv which is also a subset of GLSL (but with different
/// type and operation names). Curv distance functions must be restricted to the
/// SC subset or the SubCurv Compiler will report an error during rendering.
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
    Source_State &sstate_;
    std::unordered_map<Value, SC_Value, Value::Hash, Value::Hash_Eq>
        valcache_{};
    std::vector<Op_Cache> opcaches_{};

    SC_Compiler(std::ostream& s, SC_Target t, Source_State& ss)
    :
        out_(s), target_(t), valcount_(0), sstate_(ss)
    {
    }

    std::ostream& out()
    {
        if (in_constants_)
            return constants_;
        else
            return body_;
    }

    // This is the main entry point to the SubCurv Compiler.
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
};

// Encapsulate an SC_Value as an expression (Operation).
// This will never be evaluated; it's only used as an argument
// to Function::sc_call_expr.
struct SC_Value_Expr : public Operation
{
    SC_Value val_;
    SC_Value_Expr(Shared<const Phrase> syntax, SC_Value val)
    :
        Operation(syntax),
        val_(val)
    {
    }
    virtual void exec(Frame&, Executor&) const override;
    virtual SC_Value sc_eval(SC_Frame&) const override;
};

SC_Value sc_eval_op(SC_Frame&, const Operation& op);
SC_Value sc_eval_expr(SC_Frame&, const Operation& op, SC_Type);
SC_Value sc_eval_const(SC_Frame&, Value val, const Phrase&);
SC_Value sc_vec_element(SC_Frame&, SC_Value, int);
void sc_plex_unify(SC_Frame&, SC_Value& a, SC_Value& b, const Context& cx);
bool sc_try_extend(SC_Frame&, SC_Value& a, SC_Type b);
SC_Value sc_binop(
    SC_Frame&, SC_Type rtype, SC_Value x, const char* op, SC_Value y);
SC_Value sc_bincall(
    SC_Frame&, SC_Type rtype, const char* fn, SC_Value x, SC_Value y);
SC_Value sc_unary_call(SC_Frame&, SC_Type rtype, const char* fn, SC_Value x);

} // namespace
#endif // header guard
