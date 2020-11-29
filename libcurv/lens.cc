// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/lens.h>

#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/list.h>
#include <libcurv/num.h>
#include <libcurv/reactive.h>
#include <libcurv/symbol.h>
#include <cmath>

namespace curv {

Shared<const String> index_error(
    const String_Ref &synopsis,
    Value value, Value index,
    const Value* slice, const Value* endslice)
{
    String_Builder msg;

    msg << synopsis << "\n"
        << "value: " << value;
    if (auto rx = value.maybe<Reactive_Value>())
        msg << " (type " << rx->sctype_ << ")";
    msg << "\n";

    if (slice) {
        msg << "slice: .[" << index;
        while (slice < endslice) {
            msg << "," << *slice;
            ++slice;
        }
        msg << "]";
    } else {
        msg << "index: " << index;
    }
    return msg.get_string();
}

const Phrase& index_value_phrase(const At_Syntax& cx)
{
    // TODO: more precise
    return cx.syntax();
}

Value get_value_at_slice(
    Value value, const Value* slice, const Value* endslice,
    const At_Syntax& cx)
{
    if (slice == endslice)
        return value;
    return get_value_at_index(value, slice[0], slice+1, endslice, cx);
}

Value get_value_at_boxed_slice(Value value, Value slice, const At_Syntax& cx)
{
    auto list = slice.to<const List>(cx);
    return get_value_at_slice(value, list->begin(), list->end(), cx);
}

Value get_value_at_index(
    Value value, Value index,
    const Value* slice, const Value* endslice,
    const At_Syntax& cx)
{
    if (index.is_num()) {
        double num = index.to_num_unsafe();
        if (!num_is_int(num)) {
            throw Exception(cx, index_error(
                "Numeric index is not an integer",
                value, index, slice, endslice));
        }
        if (auto list = value.maybe<List>()) {
            if (list->empty()) {
                throw Exception(cx, index_error(
                    "List index is out of range",
                    value, index, slice, endslice));
            }
            if (num < 0.0 || num > double(list->size()-1)) {
                throw Exception(cx, index_error(
                    stringify("List index is not in range 0..",list->size()-1),
                    value, index, slice, endslice));
            }
            auto r = list->at(size_t(num));
            return get_value_at_slice(r, slice, endslice, cx);
        }
        if (auto string = value.maybe<String>()) {
            unsigned len = string->size();
            if (len == 0) {
                throw Exception(cx, index_error(
                    "String index is out of range",
                    value, index, slice, endslice));
            }
            if (num < 0.0 || num > double(len-1)) {
                throw Exception(cx, index_error(
                    stringify("String index is not in range 0..",len-1),
                    value, index, slice, endslice));
            }
            auto r = Value{make_string(string->data()+size_t(num),1)};
            return get_value_at_slice(r, slice, endslice, cx);
        }
        if (auto rx = value.maybe<Reactive_Value>()) {
            if (rx->sctype_.is_list()) {
                auto k = rx->sctype_.count();
                if (num < 0.0 || num > double(k-1)) {
                    throw Exception(cx, index_error(
                        stringify("List index is not in range 0..",k-1),
                        value, index, slice, endslice));
                }
                auto r = make<Reactive_Expression>(
                    rx->sctype_.elem_type(),
                    make<Index_Expr>(
                        share(cx.syntax()),
                        to_expr(value, cx.syntax()),
                        to_expr(index, cx.syntax())),
                    cx);
                return get_value_at_slice(Value{r}, slice, endslice, cx);
            }
        }
        throw Exception(cx, index_error(
            "Integer index into a non-list value",
            value, index, slice, endslice));
    }
    if (auto sym = maybe_symbol(index)) {
        auto rec = value.maybe<const Record>();
        if (rec == nullptr) {
            throw Exception(cx, index_error(
                "Symbol index into a non-record value",
                value, index, slice, endslice));
        }
        auto elem = rec->find_field(sym, cx);
        if (elem.is_missing()) {
            throw Exception(cx, index_error(
                "Symbol index not defined in the record value",
                value, index, slice, endslice));
        }
        return get_value_at_slice(elem, slice, endslice, cx);
    }
    if (auto list = index.maybe<List>()) {
        auto string = value.maybe<String>();
        if (string && slice==endslice) {
            String_Builder sb;
            for (auto ival : *list) {
                int i = ival.to_int(0, int(string->size()-1), cx);
                sb << string->at(i);
            }
            return {sb.get_string()};
        }
        // More complicated string slicing cases will produce weird results.
        // TODO: This is fixed once a string is a list of characters.
        Shared<List> result = List::make(list->size());
        for (unsigned i = 0; i < list->size(); ++i)
            result->at(i) =
                get_value_at_index(value, list->at(i), slice, endslice, cx);
        return {result};
    }
    if (auto func = maybe_function(index, cx)) {
        std::unique_ptr<Frame> f2 =
            Frame::make(func->nslots_, cx.system(), cx.frame(),
                share(cx.syntax()), nullptr);
        f2->func_ = func;
        auto r = func->call(value, Fail::hard, *f2);
        f2 = nullptr;
        return get_value_at_slice(r, slice, endslice, cx);
    }
    if (auto ri = index.maybe<Reactive_Value>()) {
        if (ri->sctype_.is_num()) {
            auto type = sc_type_of(value);
            if (type.is_list()) {
                auto r = make<Reactive_Expression>(
                    type.elem_type(),
                    make<Index_Expr>(
                        share(cx.syntax()),
                        to_expr(value, index_value_phrase(cx)),
                        ri->expr()),
                    cx);
                return get_value_at_slice(Value{r}, slice, endslice, cx);
            }
        } else if (ri->sctype_.is_list()) {
            // TODO
            // The only thing I need is the result type of value@index
            // and then it's the same code as above. Wait til SubCurv supports
            // this then use the function that computes the result type.
        }
    }
    throw Exception(cx, index_error(
        "Invalid index",
        value, index, slice, endslice));
}

} // namespace curv
