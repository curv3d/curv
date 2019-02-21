// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/record.h>
#include <libcurv/exception.h>

namespace curv {

const char Record::name[] = "record";

Value
Record::getfield(Symbol field, const Context& cx) const
{
    throw Exception(cx, stringify(*this," does not contain field .",field));
}

bool
Record::equal(const Record& rhs, const Context& cx) const
{
    if (this->size() != rhs.size())
        return false;
    for (auto i = iter(); !i->empty(); i->next()) {
        if (!rhs.hasfield(i->key()))
            return false;
        if (!i->value(cx).equal(rhs.getfield(i->key(),cx),cx))
            return false;
    }
    return true;
}

void
Record::each_field(
    const Context& cx, std::function<void(Symbol,Value)> visitor) const
{
    for (auto f = iter(); !f->empty(); f->next())
        visitor(f->key(), f->value(cx));
}

Shared<List>
Record::fields() const
{
    auto list = List::make(size());
    int i = 0;
    for (auto f = iter(); !f->empty(); f->next()) {
        list->at(i) = f->key().to_value();
        ++i;
    }
    return {std::move(list)};
}

std::pair<Symbol, Value>
value_to_variant(Value val, const Context& cx)
{
    Shared<Record> rec = val.to<Record>(cx);
    if (rec->size() != 1)
        throw Exception(cx, stringify(val, " is not a variant"));
    auto i = rec->iter();
    return std::pair<Symbol,Value>{i->key(), i->value(cx)};
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

} // namespace curv
