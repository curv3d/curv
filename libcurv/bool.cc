// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/bool.h>

#include <libcurv/exception.h>
#include <libcurv/reactive.h>

namespace curv {

bool is_bool(Value a)
{
    if (a.is_bool()) return true;
    auto r = a.maybe<Reactive_Value>();
    return r && r->sctype_.is_bool();
}

void assert_bool(Value a, const Context& cx)
{
    if (!is_bool(a))
        throw Exception(cx, stringify(a," is not a boolean"));
}

Shared<const List>
nat_to_bool32(unsigned n)
{
    Shared<List> result = List::make(32);
    for (unsigned i = 0; i < 32; ++i)
        result->at(i) = {(n & (1 << i)) != 0};
    return result;
}

unsigned
bool32_to_nat(Shared<const List> li, const Context& cx)
{
    unsigned out = 0;
    for (unsigned i = 0; i < 32; ++i) {
        out |= unsigned(li->at(i).to_bool(cx)) << i;
    }
    return out;
}

} // namespace curv
