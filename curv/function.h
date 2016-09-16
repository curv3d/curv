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
    Value (*function_)(Value* argv);
    unsigned nargs_;

    Function(
        Value (*fun)(Value*),
        unsigned nargs)
    :
        Ref_Value(ty_function),
        function_(fun),
        nargs_(nargs)
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

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const;
};

} // namespace curv
#endif // header guard
