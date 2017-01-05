// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/record.h>

namespace curv {

void
Record::print(std::ostream& out) const
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

auto Record::operator==(const Record& rec) const
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

auto Record::getfield(Atom name) const
-> Value
{
    auto fp = fields_.find(name);
    if (fp != fields_.end())
        return fp->second;
    return missing;
}

} // namespace curv
