// Copyright 2016-2019 Doug Moen
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
            stringify("list ",*this," does not have ",sz," elements"));
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

Shared<List> List_Base::clone() const
{
    return List::make_copy(array_, size_);
}

Value* List_Base::ref_element(Value index, bool need_value, const Context& cx)
{
    auto index_list = index.to<List>(cx);
    index_list->assert_size(1, cx);
    int i = index_list->at(0).to_int(0, int(size_)-1, cx);
    (void)need_value;
    return &array_[i];
}

} // namespace curv
