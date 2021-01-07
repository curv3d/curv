// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/list.h>

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/reactive.h>

namespace curv {

Generic_List::Generic_List(Value val, Fail fl, const Context& cx)
{
    if (val.is_ref()) {
        list_ = share(val.to_ref_unsafe());
        switch (list_->type_) {
        case Ref_Value::ty_abstract_list:
            return;
        case Ref_Value::ty_reactive:
          {
            auto* rx = (Reactive_Value*)(&*list_);
            if (rx->sctype_.is_list())
                return;
          }
        }
    }
    if (fl == Fail::hard)
        throw Exception(cx, stringify(val, " is not a list"));
    else
        list_ = nullptr;
}

size_t Generic_List::size() const noexcept
{
    if (is_abstract_list())
        return get_abstract_list().size();
    else
        return get_reactive_value().sctype_.count();
}

void Generic_List::assert_size(size_t sz, const Context& cx) const
{
    if (size() != sz)
        throw Exception(cx,
            stringify("list ",Value{list_}," does not have ",sz," elements"));
}

Value Generic_List::val_at(size_t i, const At_Syntax& cx) const
{
    if (is_abstract_list())
        return get_abstract_list().val_at(i);
    else {
        auto& rx = get_reactive_value();
        auto ph = share(cx.syntax());
        return {make<Reactive_Expression>(
            rx.sctype_.elem_type(),
            make<Index_Expr>(
                ph,
                rx.expr(),
                make<Constant>(ph, Value(double(i)))),
            cx)};
    }
}
void Generic_List::amend_at(size_t i, Value newval, const At_Syntax& cx)
{
    if (this->is_boxed_list()) {
        if (list_->use_count > 1) {
            auto& bl = get_boxed_list();
            list_ = List::make_copy(bl.begin(), bl.size());
        }
        get_boxed_list().at(i) = newval;
    }
    else if (this->is_string()) {
        if (newval.is_char()) {
            if (list_->use_count > 1) {
                auto& str = get_string();
                list_ = make_string(str.data(), str.size());
            }
            get_string().at(i) = newval.to_char_unsafe();
        } else {
            auto& str = get_string();
            auto li = List::make(str.size());
            for (unsigned j = 0; j < str.size(); ++j)
                li->at(j) = Value{str[j]};
            li->at(i) = newval;
            list_ = std::move(li);
        }
    }
    else if (this->is_reactive_value())
        throw Exception(cx, "Generic_List: can't amend symbolic list");
    else
        throw Exception(cx, "Generic_List: internal error in amend_at");
}

const char List_Base::name[] = "list";
const char Abstract_List::name[] = "list";

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
