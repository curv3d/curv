// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_FUNCTION_H
#define CURV_FUNCTION_H

#include <curv/value.h>
#include <curv/meaning.h>
#include <curv/list.h>
#include <curv/gl_compiler.h>

namespace curv {

/// A function value.
struct Function : public Ref_Value
{
    slot_t nslots_; // size of call frame

    Function(slot_t nslots)
    :
        Ref_Value(ty_function),
        nslots_(nslots)
    {}

    // call the function during evaluation
    virtual Value call(Value, Frame&) = 0;

    // Attempt a function call: return `missing` if the parameter pattern
    // doesn't match the value; otherwise call the function and return result.
    virtual Value try_call(Value, Frame&) = 0;

    // Generate a call to the function during geometry compilation.
    // The argument is represented as an expression.
    virtual GL_Value gl_call_expr(Operation&, const Call_Phrase*, GL_Frame&) const;

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;

    static const char name[];
};

/// A polyadic function value.
/// It has a fixed number of arguments (nargs), the same number on every call.
/// Within `call(Frame& args)`, use `args[i]` to fetch the i'th argument.
struct Polyadic_Function : public Function
{
    unsigned nargs_;

    Polyadic_Function(unsigned nargs)
    :
        Function(nargs),
        nargs_(nargs)
    {}
    Polyadic_Function(unsigned nargs, unsigned nslots)
    :
        Function(nslots),
        nargs_(nargs)
    {}

    // call the function during evaluation, with specified argument value.
    virtual Value call(Value, Frame&) override;
    virtual Value try_call(Value, Frame&) override;

    // call the function during evaluation, with arguments stored in the frame.
    virtual Value call(Frame& args) = 0;

    // Generate a call to the function during geometry compilation.
    // The argument is represented as an expression.
    virtual GL_Value gl_call_expr(Operation&, const Call_Phrase*, GL_Frame&) const override;

    // generate a call to the function during geometry compilation
    virtual GL_Value gl_call(GL_Frame&) const;
};

/// The run-time representation of a compiled lambda expression.
///
/// This is the compile-time component of a function value, minus the
/// values of non-local variables, which are captured at run time in a Closure.
/// It's not a proper value, but can be stored in a Value slot.
struct Lambda : public Ref_Value
{
    Shared<const Pattern> pattern_;
    Shared<Operation> expr_;
    slot_t nslots_; // size of call frame

    Lambda(
        Shared<const Pattern> pattern,
        Shared<Operation> expr,
        slot_t nslots)
    :
        Ref_Value(ty_lambda),
        pattern_(std::move(pattern)),
        expr_(std::move(expr)),
        nslots_(nslots)
    {}

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
};

/// A user-defined function value,
/// represented by a closure over a lambda expression.
struct Closure : public Function
{
    Shared<const Pattern> pattern_;
    Shared<Operation> expr_;
    Shared<Module> nonlocals_;

    Closure(
        Shared<const Pattern> pattern,
        Shared<Operation> expr,
        Shared<Module> nonlocals,
        slot_t nslots)
    :
        Function(nslots),
        pattern_(std::move(pattern)),
        expr_(std::move(expr)),
        nonlocals_(std::move(nonlocals))
    {}

    Closure(
        Lambda& lambda,
        const Module& nonlocals)
    :
        Function(lambda.nslots_),
        pattern_(lambda.pattern_),
        expr_(lambda.expr_),
        nonlocals_(share(const_cast<Module&>(nonlocals)))
    {}

    virtual Value call(Value, Frame&) override;
    virtual Value try_call(Value, Frame&) override;

    // generate a call to the function during geometry compilation
    virtual GL_Value gl_call_expr(Operation&, const Call_Phrase*, GL_Frame&) const override;
};

struct Piecewise_Function : public Function
{
    std::vector<Shared<Function>> cases_;

    static slot_t maxslots(std::vector<Shared<Function>>&);

    Piecewise_Function(std::vector<Shared<Function>> cases)
    :
        Function(maxslots(cases)),
        cases_(std::move(cases))
    {}

    // call the function during evaluation, with specified argument value.
    virtual Value call(Value, Frame&) override;
    virtual Value try_call(Value, Frame&) override;
};

} // namespace curv
#endif // header guard
