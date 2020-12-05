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
    if (a.maybe<Abstract_List>())
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
List_Base::print_string(std::ostream& out) const
{
    bool in_string = true;
    bool first_after_left_bracket = false;
    for (auto e : *this) {
        if (e.is_char()) {
            if (!in_string) {
                out << ']';
                in_string = true;
            }
            out << e.to_char_unsafe();
        } else {
            if (in_string) {
                out << '[';
                in_string = false;
                first_after_left_bracket = true;
            }
            if (!first_after_left_bracket)
                out << ',';
            e.print_repr(out);
            first_after_left_bracket = false;
        }
    }
    if (!in_string)
        out << ']';
}
void
List_Base::print_repr(std::ostream& out) const
{
    enum {begin, in_string, in_list} state = begin;
    bool first_after_left_bracket = false;
    for (size_t i = 0; i < size(); ++i) {
        Value e = array_[i];
        if (e.is_char()) {
            switch (state) {
            case begin: out << '"'; break;
            case in_string: break;
            case in_list: out << "]++\""; break;
            }
            state = in_string;
            char next = 0;
            if (i + 1 < size() && array_[i+1].is_char())
                next = array_[i+1].to_char_unsafe();
            write_curv_char(e.to_char_unsafe(), next, 0, out);
        } else {
            switch (state) {
            case begin:
                out << '[';
                first_after_left_bracket = true;
                break;
            case in_string:
                out << "\"++[";
                first_after_left_bracket = true;
                break;
            case in_list:
                break;
            }
            state = in_list;
            if (!first_after_left_bracket)
                out << ',';
            first_after_left_bracket = false;
            e.print_repr(out);
        }
    }
    switch (state) {
    case begin: out << "[]"; break;
    case in_string: out << '"'; break;
    case in_list: out << ']'; break;
    }
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

void List_Builder::push_back(Value val)
{
    if (in_string_) {
        if (val.is_char()) {
            string_.push_back(val.to_char_unsafe());
            return;
        }
        for (auto c : string_)
            list_.push_back({c});
        in_string_ = false;
    }
    list_.push_back(val);
}

void List_Builder::concat(Value val, const Context& cx)
{
    if (auto strval = val.maybe<String>()) {
        // Strings can't be empty.
        if (in_string_)
            string_ += strval->c_str();
        else {
            for (auto c : *strval)
                list_.push_back({c});
        }
    } else if (auto listval = val.maybe<List>()) {
        if (listval->empty()) return;
        // A non-empty List is guaranteed to contain 1 non-character,
        // so we need to switch out of string mode.
        if (in_string_) {
            for (auto c : string_)
                list_.push_back({c});
            in_string_ = false;
        }
        list_.insert(list_.end(), listval->begin(), listval->end());
    } else {
        throw Exception(cx, stringify(val, "is not a list"));
    }
}

Value List_Builder::get_value()
{
    if (in_string_) {
        if (string_.empty())
            return {List::make(0)};
        return {make_string(string_)};
    }
    Shared<List> result = List::make_elements(list_);
    return {result};
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

Value* List_Base::ref_lens(Value lens, bool need_value, const Context& cx)
{
    int i = lens.to_int(0, int(size_)-1, cx);
    (void)need_value;
    return &array_[i];
}

} // namespace curv
