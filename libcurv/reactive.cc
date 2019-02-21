// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/reactive.h>
#include <libcurv/exception.h>
#include <boost/core/demangle.hpp>

namespace curv {

Reactive_Expression::Reactive_Expression(
    GL_Type gltype,
    Shared<Operation> expr,
    const Context& cx)
:
    Reactive_Value(sty_reactive_expression, gltype),
    expr_(std::move(expr))
{
    if (!expr_->pure_) {
        const Operation& exp { *expr };
        throw Exception{cx, stringify(
            "reactive value constructed from impure expression ",
            boost::core::demangle(typeid(exp).name()))};
    }
}

void
Reactive_Value::print(std::ostream& out) const
{
    out << "<reactive>";
}

Shared<Operation>
Reactive_Value::expr(const Phrase& syntax)
{
    return make<Constant>(share(syntax), Value{share(*this)});
}

Shared<Operation>
Reactive_Expression::expr(const Phrase& syntax)
{
    return expr_;
}

} // namespace curv
