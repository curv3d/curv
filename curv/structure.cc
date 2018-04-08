// Copyright 2017-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <curv/structure.h>
#include <curv/exception.h>

namespace curv {

const char Structure::name[] = "record";

Value
Structure::getfield(Atom field, const Context& cx) const
{
    throw Exception(cx, stringify(".",field,": not defined"));
}

} // namespace curv
