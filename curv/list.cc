// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

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

auto curv::List_Builder::get_list()
-> Shared<List>
{
    return List::make_elements(*this);
}
