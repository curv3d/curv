// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/record.h>

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

auto Record::operator==(const Record& rec) const
-> bool
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
    return Ref_Value::getfield(name, cx);
}

bool
Record::hasfield(Atom name) const
{
    auto fp = fields_.find(name);
    return (fp != fields_.end());
}

} // namespace curv
