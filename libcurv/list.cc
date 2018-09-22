// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/list.h>
#include <libcurv/exception.h>

namespace curv {

const char List_Base::name[] = "list";

void
List_Base::assert_size(size_t sz, const Context& cx)
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

auto List_Base::equal(const List_Base& list, const Context& cx) const
-> bool
{
    if (size() != list.size())
        return false;
    for (size_t i = 0; i < size(); ++i) {
        if (!array_[i].equal(list.array_[i], cx))
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
