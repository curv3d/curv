// Copyright 2017-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/structure.h>
#include <libcurv/exception.h>

namespace curv {

const char Structure::name[] = "record";

Value
Structure::getfield(Symbol field, const Context& cx) const
{
    throw Exception(cx, stringify(".",field,": not defined"));
}

bool
Structure::equal(const Structure& rhs, const Context& cx) const
{
    // TODO: This is not efficient. There is no way to short-circuit
    // the 'each_field' loop. We need a Structure iterator.
    if (this->size() != rhs.size())
        return false;
    bool r = true;
    this->each_field([&](Symbol sym, Value val) {
        if (!rhs.hasfield(sym))
            r = false;
        else if (!val.equal(rhs.getfield(sym,cx),cx))
            r = false;
    });
    return r;
}

} // namespace curv
