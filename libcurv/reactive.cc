// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/reactive.h>
#include <libcurv/exception.h>
#include <boost/core/demangle.hpp>

namespace curv {

Ternary Reactive_Value::equal(Value val) const
{
#if 0
    if (val.is_ref()) {
        Reactive_Value &rx = val.to_ref_unsafe();
        if (type_.intersects(rx.type_))
            return Ternary::Unknown;
        else
            return Ternary::False;
    }
    if (type_.contains(val))
        return Ternary::Unknown;
    else
        return Ternary::False;
#endif
    return Ternary::Unknown;
}

Reactive_Expression::Reactive_Expression(
    SC_Type sctype,
    Shared<Operation> expr,
    const Context& cx)
:
    Reactive_Value(sty_reactive_expression, sctype),
    expr_(std::move(expr))
{
    if (!expr_->pure_) {
        const Operation& exp { *expr };
        throw Exception{cx, stringify(
            "reactive value constructed from impure expression ",
            boost::core::demangle(typeid(exp).name()))};
    }
}

void Reactive_Value::print_repr(std::ostream& out) const
{
    out << "<reactive:" << sctype_ << ">";
}

void Reactive_Expression::print_repr(std::ostream& out) const
{
    out << *expr_;
}

Shared<Operation> Reactive_Expression::expr() const
{
    return expr_;
}

Shared<Operation> to_expr(Value val, const Phrase& syn)
{
    auto rx = val.maybe<Reactive_Value>();
    if (rx)
        return rx->expr();
    else
        return make<Constant>(share(syn), val);
}

} // namespace curv
