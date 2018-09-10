// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/record.h>
#include <libcurv/exception.h>

namespace curv {

const char Record::name[] = "record";

Value
Record::getfield(Symbol field, const Context& cx) const
{
    throw Exception(cx, stringify(".",field,": not defined"));
}

bool
Record::equal(const Record& rhs, const Context& cx) const
{
    // TODO: This is not efficient. There is no way to short-circuit
    // the 'each_field' loop. We need a Record iterator.
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
    return Record::getfield(name, cx);
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
