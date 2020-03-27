// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/list.h>

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/reactive.h>

namespace curv {

bool is_list(Value a)
{
    if (a.maybe<List>())
        return true;
    if (auto r = a.maybe<Reactive_Value>())
        return r->sctype_.is_list();
    return false;
}
size_t list_count(Value val)
{
    if (val.is_ref()) {
        auto& ref = val.to_ref_unsafe();
        switch (ref.type_) {
        case Ref_Value::ty_list:
          {
            auto list = (List*)&ref;
            return list->size();
          }
        case Ref_Value::ty_reactive:
          {
            auto rx = (Reactive_Value*)&ref;
            if (rx->sctype_.is_list())
                return rx->sctype_.count();
          }
        }
    }
    return 1;
}
Value list_elem(Value val, size_t i, const At_Syntax& cx)
{
    if (val.is_ref()) {
        auto& ref = val.to_ref_unsafe();
        switch (ref.type_) {
        case Ref_Value::ty_list:
          {
            auto list = (List*)&ref;
            return list->at(i);
          }
        case Ref_Value::ty_reactive:
          {
            auto rx = (Reactive_Value*)&ref;
            auto ph = share(cx.syntax());
            Shared<List_Expr> index = List_Expr::make(
                {make<Constant>(ph, Value(double(i)))},
                ph);
            index->init();
            return {make<Reactive_Expression>(
                rx->sctype_.elem_type(),
                make<Call_Expr>(ph, rx->expr(), index),
                cx)};
          }
        }
    }
    return {};
}

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
List_Base::print_repr(std::ostream& out) const
{
    out << "[";
    for (size_t i = 0; i < size(); ++i) {
        if (i > 0) out << ",";
        array_[i].print_repr(out);
    }
    out << "]";
}

Ternary List_Base::equal(const List_Base& list, const Context& cx) const
{
    if (size() != list.size())
        return Ternary::False;
    Ternary result = Ternary::True;
    for (size_t i = 0; i < size(); ++i) {
        Ternary ter = array_[i].equal(list.array_[i], cx);
        if (ter == Ternary::False) return Ternary::False;
        result &= ter;
    }
    return result;
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
