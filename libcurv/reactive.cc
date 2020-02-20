// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/reactive.h>
#include <libcurv/exception.h>
#include <boost/core/demangle.hpp>

namespace curv {

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

void Reactive_Value::print(std::ostream& out) const
{
    out << "<reactive:" << sctype_ << ">";
}

void Reactive_Expression::print(std::ostream& out) const
{
    out << *expr_;
}

Shared<Operation> Reactive_Expression::expr() const
{
    return expr_;
}

} // namespace curv
