// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/record.h>

namespace curv {

const char DRecord::name[] = "record";

void
DRecord::print(std::ostream& out) const
{
    out << "{";
    bool first = true;
    for (auto i : fields_) {
        if (!first) out << ",";
        first = false;
        out << i.first << ":";
        i.second.print(out);
    }
    out << "}";
}

void
DRecord::putfields(Symbol_Map<Value>& out) const
{
    for (auto i : fields_)
        out[i.first] = i.second;
}

Value
DRecord::getfield(Symbol name, const Context& cx) const
{
    auto fp = fields_.find(name);
    if (fp != fields_.end())
        return fp->second;
    return Structure::getfield(name, cx);
}

bool
DRecord::hasfield(Symbol name) const
{
    auto fp = fields_.find(name);
    return (fp != fields_.end());
}

Shared<List>
DRecord::fields() const
{
    auto list = List::make(fields_.size());
    int i = 0;
    for (auto f : fields_) {
        list->at(i) = f.first.to_value();
        ++i;
    }
    return {std::move(list)};
}

void
DRecord::each_field(std::function<void(Symbol,Value)> visitor) const
{
    for (auto f : fields_)
        visitor(f.first, f.second);
}

} // namespace curv
