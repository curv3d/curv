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
    virtual Value call(Frame& args) = 0;

    Function(
        unsigned nargs)
    :
        Ref_Value(ty_function),
        nargs_(nargs)
    {}

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
    size_t nargs_;
    size_t nslots_; // size of call frame

    Lambda(
        Shared<const Operation> expr,
        size_t nargs, size_t nslots)
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
struct Closure : public Ref_Value
{
    Shared<const Operation> expr_;
    Shared<List> nonlocals_;
    size_t nargs_;
    size_t nslots_; // size of call frame

    Closure(
        Shared<const Operation> expr,
        Shared<List> nonlocals,
        size_t nargs, size_t nslots)
    :
        Ref_Value(ty_closure),
        expr_(std::move(expr)),
        nonlocals_(std::move(nonlocals)),
        nargs_(nargs),
        nslots_(nslots)
    {}

    Closure(
        Lambda& lambda,
        List& nonlocals)
    :
        Ref_Value(ty_closure),
        expr_(lambda.expr_),
        nonlocals_(share(nonlocals)),
        nargs_(lambda.nargs_),
        nslots_(lambda.nslots_)
    {}

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
};

} // namespace curv
#endif // header guard
