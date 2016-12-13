// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_FUNCTION_H
#define CURV_FUNCTION_H

#include <curv/value.h>
#include <curv/meaning.h>
#include <curv/list.h>

namespace curv {

/// A function value.
///
/// Functions have a fixed number of arguments, specified by nargs.
/// Within the `call` function, use `args[i]` to fetch the i'th argument.
struct Function : public Ref_Value
{
    unsigned nargs_;
    unsigned nslots_; // size of call frame

    Function(unsigned nargs)
    :
        Ref_Value(ty_function),
        nargs_(nargs),
        nslots_(nargs)
    {}
    Function(unsigned nargs, unsigned nslots)
    :
        Ref_Value(ty_function),
        nargs_(nargs),
        nslots_(nslots)
    {}

    virtual Value call(Frame& args) = 0;

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
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
struct Closure : public Function
{
    Shared<const Operation> expr_;
    Shared<List> nonlocal_;

    Closure(
        Shared<const Operation> expr,
        Shared<List> nonlocal,
        unsigned nargs, unsigned nslots)
    :
        Function(nargs, nslots),
        expr_(std::move(expr)),
        nonlocal_(std::move(nonlocal))
    {}

    Closure(
        Lambda& lambda,
        List& nonlocal)
    :
        Function(lambda.nargs_, lambda.nslots_),
        expr_(lambda.expr_),
        nonlocal_(share(nonlocal))
    {}

    virtual Value call(Frame& args) override;
};

} // namespace curv
#endif // header guard
