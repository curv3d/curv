// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/list.h>
#include <curv/exception.h>

namespace curv {

const char List_Base::name[] = "list";

void
List_Data::assert_size(size_t sz, const Context& cx)
const
{
    if (size() != sz)
        throw Exception(cx,
            stringify("list has wrong size: expected ",sz,", got ",size()));
}

void
List_Base::print(std::ostream& out) const
{
    out << "[";
    for (size_t i = 0; i < size(); ++i) {
        if (i > 0) out << ",";
        array_[i].print(out);
    }
    out << "]";
}

auto List_Base::operator==(const List_Base& list) const
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

auto List_Builder::get_list()
-> Shared<List>
{
    return List::make_elements(*this);
}

} // namespace curv
