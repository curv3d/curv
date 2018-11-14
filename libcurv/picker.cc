// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/picker.h>
#include <libcurv/exception.h>

namespace curv {

void
Picker::Config::write(std::ostream& out)
{
    switch (type_) {
    case Type::slider:
        out << "slider(" << slider_.low_ << "," << slider_.high_ << ")";
        break;
    default:
        out << "bad picker config type " << int(type_);
        break;
    }
}

void
Picker::State::write(std::ostream& out, Type type)
{
    switch (type) {
    case Type::slider:
        out << slider_;
        break;
    default:
        out << "bad picker config type " << int(type);
        break;
    }
}

Picker::State::State(Type type, Value val, const Context& cx)
{
    switch (type) {
    case Type::slider:
        slider_ = val.to_num(cx);
        break;
    default:
        throw Exception{cx, stringify("bad picker type ", int(type))};
    }
}

} // namespace curv
