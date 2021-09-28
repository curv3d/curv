// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_REACTIVE_H
#define LIBCURV_REACTIVE_H

#include <libcurv/sc_type.h>
#include <libcurv/function.h> // prereq of meaning.h
#include <libcurv/meaning.h>

namespace curv {

// A reactive value changes over time. Same idea as a Behaviour in functional
// reactive programming. An abstract class.
struct Reactive_Value : public Ref_Value
{
    SC_Type sctype_;

    Reactive_Value(
        int subty, SC_Type sctype)
    :
        Ref_Value(ty_reactive, subty),
        sctype_(sctype)
    {}

    virtual void print_repr(std::ostream&) const override;
    virtual Shared<Operation> expr() const = 0;
    Ternary equal(Value) const;
    virtual void print_help(std::ostream&) const override;
};

// An expression over one or more reactive variables. Essentially, this is a
// lazy evaluation thunk. Reactive expressions can only be evaluated in a
// context where the values of their reactive variables are known, eg on the
// GPU while displaying an animated shape. In other contexts, evaluation is
// deferred.
struct Reactive_Expression : public Reactive_Value
{
    Shared<Operation> expr_;

    Reactive_Expression(SC_Type, Shared<Operation> expr, const Context&);

    virtual void print_repr(std::ostream&) const override;
    virtual Shared<Operation> expr() const override;

    size_t hash() const noexcept
    {
        return expr_->hash();
    }
    bool hash_eq(const Reactive_Expression& re) const noexcept
    {
        return expr_->hash_eq(*re.expr_);
    }
};

Shared<Operation> to_expr(Value, const Phrase&);

} // namespace curv
#endif // header guard
