// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/list.h>

void
curv::List_Base::print(std::ostream& out) const
{
    out << "[";
    for (size_t i = 0; i < size(); ++i) {
        if (i > 0) out << ",";
        array_[i].print(out);
    }
    out << "]";
}

auto curv::List_Base::operator==(const List_Base& list) const
-> bool
{
    if (size() != list.size())
        return false;
    for (size_t i = 0; i < size(); ++i) {
        if (array_[i] != list.array_[i])
            return false;
    }
    return true;
}
