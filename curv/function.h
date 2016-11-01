// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_FUNCTION_H
#define CURV_FUNCTION_H

#include <curv/value.h>
#include <curv/meaning.h>
#include <curv/list.h>

namespace curv {

/// A built-in function value, represented by a boxed native C function.
struct Function : public Ref_Value
{
    Value (*function_)(Frame& f);
    unsigned nargs_;

    Function(
        Value (*fun)(Frame&),
        unsigned nargs)
    :
        Ref_Value(ty_function),
        function_(fun),
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
    Shared<const Expression> expr_;
    size_t nargs_;
    size_t nslots_; // size of call frame

    Lambda(
        Shared<const Expression> expr,
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
    Shared<const Expression> expr_;
    Shared<List> nonlocals_;
    size_t nargs_;
    size_t nslots_; // size of call frame

    Closure(
        Shared<const Expression> expr,
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
        nonlocals_(Shared<List>(&nonlocals)),
        nargs_(lambda.nargs_),
        nslots_(lambda.nslots_)
    {}

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
};

} // namespace curv
#endif // header guard
