// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/reactive.h>
#include <libcurv/exception.h>
#include <libcurv/meanings.h>
#include <boost/core/demangle.hpp>

namespace curv {

Ternary Reactive_Value::equal(Value val) const
{
    SC_Type ty = sc_type_of(val);
    if (ty == sctype_)
        return Ternary::Unknown;
    else
        return Ternary::False;
}

void Reactive_Value::print_help(std::ostream& out) const
{
    out << "Reactive value, type=" << sctype_ << "\n";
}

Reactive_Expression::Reactive_Expression(
    SC_Type sctype,
    Shared<Operation> expr,
    const Context& cx)
:
    Reactive_Value(sty_reactive_expression, sctype),
    expr_(move(expr))
{
    if (!expr_->pure_) {
        const Operation& exp { *expr };
        throw Exception{cx, stringify(
            "reactive value constructed from impure expression ",
            boost::core::demangle(typeid(exp).name()))};
    }
}

void Reactive_Value::print_repr(std::ostream& out, Prec) const
{
    out << "<reactive:" << sctype_ << ">";
}

void Reactive_Expression::print_repr(std::ostream& out, Prec) const
{
    // TODO: use expr_->print_repr(out, rprec)
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
