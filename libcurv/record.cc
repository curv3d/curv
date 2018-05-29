// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/record.h>

namespace curv {

const char Record::name[] = "record";

void
Record::print(std::ostream& out) const
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
Record::putfields(Atom_Map<Value>& out) const
{
    for (auto i : fields_)
        out[i.first] = i.second;
}

bool
Record::operator==(const Record& rec) const
{
    auto i1 = fields_.begin();
    auto i2 = rec.fields_.begin();
    while (i1 != fields_.end()) {
        if (i2 == rec.fields_.end())
            return false;
        if (i1->first != i2->first)
            return false;
        if (i1->second != i2->second)
            return false;
        ++i1;
        ++i2;
    }
    return true;
}

Value
Record::getfield(Atom name, const Context& cx) const
{
    auto fp = fields_.find(name);
    if (fp != fields_.end())
        return fp->second;
    return Structure::getfield(name, cx);
}

bool
Record::hasfield(Atom name) const
{
    auto fp = fields_.find(name);
    return (fp != fields_.end());
}

Shared<List>
Record::fields() const
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
Record::each_field(std::function<void(Atom,Value)> visitor) const
{
    for (auto f : fields_)
        visitor(f.first, f.second);
}

} // namespace curv
