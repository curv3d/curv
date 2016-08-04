// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/record.h>

void
curv::Record::print(std::ostream& out) const
{
    out << "{";
    bool first = true;
    for (auto i : fields_) {
        if (!first) out << ",";
        first = false;
        out << i.first << "=";
        i.second.print(out);
    }
    out << "}";
}

auto curv::Record::operator==(const Record& rec) const
-> bool
{
    auto i1{fields_.begin()};
    auto i2{rec.fields_.begin()};
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
