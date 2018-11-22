// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/reactive.h>

namespace curv {

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
