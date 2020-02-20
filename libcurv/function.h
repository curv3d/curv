// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_FUNCTION_H
#define LIBCURV_FUNCTION_H

#include <libcurv/value.h>
#include <libcurv/meaning.h>
#include <libcurv/list.h>
#include <libcurv/sc_frame.h>

namespace curv {

struct Context;

/// A function value.
struct Function : public Ref_Value
{
    // size of call frame
    slot_t nslots_;

    // optional name of function
    Symbol_Ref name_{};

    // Suppose this function is the result of partial application of a named
    // function. Then this is the # of arguments that were applied to get here,
    // and `name_` is the name of the base function.
    int argpos_ = 0;

    Function(slot_t nslots)
    :
        Ref_Value(ty_function),
        nslots_(nslots)
    {}

    Function(slot_t nslots, const char* name)
    :
        Function(nslots)
    {
        name_ = make_symbol(name);
    }

    Function(const char* name)
    :
        Function(0, name)
    {
    }

    // call the function during evaluation
    virtual Value call(Value, Frame&) = 0;
    virtual void tail_call(Value, std::unique_ptr<Frame>&);

    // Attempt a function call: return `missing` if the parameter pattern
    // doesn't match the value; otherwise call the function and return result.
    virtual Value try_call(Value, Frame&);
    virtual bool try_tail_call(Value, std::unique_ptr<Frame>&);

    // Generate a call to the function during geometry compilation.
    // The argument is represented as an expression.
    virtual SC_Value sc_call_expr(Operation&, Shared<const Phrase>, SC_Frame&) const;

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;

    static const char name[];
};

// Returns nullptr if argument is not a function.
// If the Value is a record with a `call` field, then we cast the value
// of the `call` field to a function by recursively calling `cast_to_function`.
// May throw an exception if fetching the `call` field fails (currently
// only happens for directory records).
Shared<const Function> cast_to_function(Value, const Context&);

// Call a function or index into a list or string.
// Implements the Curv juxtaposition operator: `func arg`.
Value call_func(
    Value func, Value arg, Shared<const Phrase> call_phrase, Frame& f);

/// A legacy function value. Only used for builtin functions.
/// Deprecated. Builtin functions should be derived from Function.
/// Legacy functions with 0, 1 and 2 arguments are called like this:
/// f(), f(x), f(x,y). They have a single argument,
/// which is a fixed size list of count `nargs` if `nargs` != 1.
/// Within `call(Frame& args)`, use `args[i]` to fetch the i'th argument.
struct Legacy_Function : public Function
{
    unsigned nargs_;

    Legacy_Function(unsigned nargs, const char* name)
    :
        Function(nargs, name),
        nargs_(nargs)
    {}
    Legacy_Function(unsigned nargs, unsigned nslots)
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
    virtual SC_Value sc_call_expr(Operation&, Shared<const Phrase>, SC_Frame&) const override;

    // generate a call to the function during geometry compilation
    virtual SC_Value sc_call_legacy(SC_Frame&) const;
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

    // optional name of function
    Symbol_Ref name_{};

    // Suppose this function is the result of partial application of a named
    // function. Then this is the # of arguments that were applied to get here,
    // and `name_` is the name of the base function.
    int argpos_ = 0;

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
    {
        name_ = lambda.name_;
        argpos_ = lambda.argpos_;
    }

    virtual Value call(Value, Frame&) override;
    virtual void tail_call(Value, std::unique_ptr<Frame>&) override;
    virtual Value try_call(Value, Frame&) override;
    virtual bool try_tail_call(Value, std::unique_ptr<Frame>&) override;

    // generate a call to the function during geometry compilation
    virtual SC_Value sc_call_expr(Operation&, Shared<const Phrase>, SC_Frame&) const override;
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
    virtual void tail_call(Value, std::unique_ptr<Frame>&) override;
    virtual Value try_call(Value, Frame&) override;
    virtual bool try_tail_call(Value, std::unique_ptr<Frame>&) override;

    // generate a call to the function during geometry compilation
    virtual SC_Value sc_call_expr(Operation&, Shared<const Phrase>, SC_Frame&) const override;
};

} // namespace curv
#endif // header guard
