// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

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
    unsigned nslots_; // size of call frame

    Function(unsigned nslots)
    :
        Ref_Value(ty_function),
        nslots_(nslots)
    {}

    // call the function during evaluation
    virtual Value call(Value, Frame&) = 0;

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

    // call the function during evaluation, with arguments stored in the frame.
    virtual Value call(Frame& args) = 0;

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
    Shared<const Operation> expr_;
    unsigned nargs_;
    unsigned nslots_; // size of call frame

    Lambda(
        Shared<const Operation> expr,
        unsigned nargs, unsigned nslots)
    :
        Ref_Value(ty_lambda),
        expr_(std::move(expr)),
        nargs_(nargs),
        nslots_(nslots)
    {}

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
};

/// A user-defined function value,
/// represented by a closure over a lambda expression.
struct Closure : public Polyadic_Function
{
    Shared<const Operation> expr_;
    Shared<List> nonlocal_;

    Closure(
        Shared<const Operation> expr,
        Shared<List> nonlocal,
        unsigned nargs, unsigned nslots)
    :
        Polyadic_Function(nargs, nslots),
        expr_(std::move(expr)),
        nonlocal_(std::move(nonlocal))
    {}

    Closure(
        Lambda& lambda,
        List& nonlocal)
    :
        Polyadic_Function(lambda.nargs_, lambda.nslots_),
        expr_(lambda.expr_),
        nonlocal_(share(nonlocal))
    {}

    virtual Value call(Frame& args) override;

    // generate a call to the function during geometry compilation
     virtual GL_Value gl_call(GL_Frame&) const override;
};

} // namespace curv
#endif // header guard
