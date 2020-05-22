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

Shared<const String> lens_error(Value lens, Value source, const String_Ref &msg)
{
    return stringify(msg, "\nSource: ", source, "\nLens: ", lens);
}

Value lens_get(Value source, Value lens, const At_Syntax& cx)
{
    if (lens.is_num()) {
        double num = lens.to_num_unsafe();
        if (!num_is_int(num)) {
            throw Exception(cx, lens_error(lens, source, stringify(
                "List index ",num," is not an integer")));
        }
        auto list = source.maybe<List>();
        if (list == nullptr) {
            throw Exception(cx, lens_error(lens, source,
                 "The lens (a list index) "
                 "expects the source to be a list"));
        }
        if (list->empty()) {
            throw Exception(cx, lens_error(lens, source,
                stringify("List index ",num," is out of range")));
        }
        if (num < 0.0 || num > double(list->size()-1)) {
            throw Exception(cx, lens_error(lens, source,
                stringify("List index ",num,
                    " is not in range 0..",list->size()-1)));
        }
        return list->at(size_t(num));
    }
    else if (auto sym = maybe_symbol(lens)) {
        auto rec = source.maybe<const Record>();
        if (rec == nullptr) {
            throw Exception(cx, lens_error(lens, source,
                 "The lens (a field name) "
                 "expects the source to be a record"));
        }
        auto elem = rec->find_field(sym, cx);
        if (elem.is_missing()) {
            throw Exception(cx, lens_error(lens, source, stringify(
                "Field ",sym," is not defined in the source record")));
        }
        return elem;
    }
    else if (auto list = lens.maybe<List>()) {
        Shared<List> result = List::make(list->size());
        for (unsigned i = 0; i < list->size(); ++i)
            result->at(i) = lens_get(source, list->at(i), cx);
        return {result};
    }
    else if (auto func = maybe_function(lens, cx)) {
        std::unique_ptr<Frame> f2 =
            Frame::make(func->nslots_, cx.system(), cx.frame(),
                share(cx.syntax()), nullptr);
        f2->func_ = func;
        func->tail_call(source, f2);
        return tail_eval_frame(std::move(f2));
    }
    throw Exception(cx, stringify(lens," is not a lens"));
}

#if 0
Value slice_get(Value val, const List& slice, const At_Syntax& cx)
{
}
#endif
#if 0
struct Index_State
{
    Shared<const Phrase> callph_;
    At_Phrase cx;
    At_Phrase icx;

    Index_State(
        Shared<const Phrase> callph,
        Frame& f)
    :
        callph_(callph),
        cx(*callph, f),
        icx(*arg_part(callph), f)
    {}

    Shared<const Phrase> ph() { return callph_; }
    Shared<const Phrase> iph() { return arg_part(callph_); }
    Shared<const Phrase> lph() { return func_part(callph_); }

    Shared<const String> err(Value list, Value index,
        const Value* path, const Value* endpath)
    {
        String_Builder msg;
        msg << "indexing error\n";
        msg << "left side: " << list;
        if (auto rx = list.maybe<Reactive_Value>())
            msg << " (type " << rx->sctype_ << ")";
        msg << "\nright side: [" << index;
        while (path < endpath) {
            msg << "," << *path;
            ++path;
        }
        msg << "]";
        return msg.get_string();
    }
};
Value value_at_path(Value, const Value*, const Value*, Index_State&);
Value list_at_path(const List& list, Value index,
    const Value* path, const Value* endpath, Index_State& state)
{
    if (index.is_num()) {
        int i = index.to_int(0, int(list.size()-1), state.icx);
        return value_at_path(list.at(i), path, endpath, state);
    }
    else if (auto indices = index.maybe<List>()) {
        Shared<List> result = List::make(indices->size());
        int j = 0;
        for (auto ival : *indices)
            (*result)[j++] = list_at_path(list, ival, path, endpath, state);
        return {result};
    }
    else if (auto ri = index.maybe<Reactive_Value>()) {
        if (ri->sctype_.is_num()) {
            Value val = {share(list)};
            auto type = sc_type_of(val);
            if (type.is_list()) {
                Shared<List_Expr> index =
                    List_Expr::make({ri->expr()}, state.iph());
                index->init();
                Value rx = {make<Reactive_Expression>(
                    type.elem_type(),
                    make<Call_Expr>(
                        state.ph(),
                        make<Constant>(state.lph(), val),
                        index),
                    state.cx)};
                return value_at_path(rx, path+1, endpath, state);
            }
        }
        /* TODO: add general support for A[[i,j,k]] to SubCurv
        else if (ri->sc_type_.is_num_vec()) {
            ...
        }
        */
    }
    throw Exception(state.cx, state.err({share(list)}, index, path, endpath));
}
Value value_at_path(Value val, const Value* path, const Value* endpath,
    Index_State& state)
{
    if (path == endpath) return val;
    Value index = path[0];
    if (auto list = val.maybe<List>()) {
        return list_at_path(*list, index, path+1, endpath, state);
    }
    else if (auto string = val.maybe<String>()) {
        if (path+1 == endpath) {
            // TODO: this code only works for ASCII strings.
            if (index.is_num()) {
                int i = index.to_int(0, int(string->size()-1), state.icx);
                return {make_string(string->data()+i, 1)};
            }
            else if (auto indices = index.maybe<List>()) {
                String_Builder sb;
                for (auto ival : *indices) {
                    int i = ival.to_int(0, int(string->size()-1), state.icx);
                    sb << string->at(i);
                }
                return {sb.get_string()};
            }
            // reactive index not supported because String is not in SubCurv
        }
    }
    else if (auto rx = val.maybe<Reactive_Value>()) {
        // TODO: what to do for pathsize > 1? punt for now.
        if (path+1 == endpath && is_num(index)) {
            auto iph = state.iph();
            auto lph = state.lph();
            Shared<List_Expr> ix = List_Expr::make({to_expr(index,*iph)}, iph);
            ix->init();
            return {make<Reactive_Expression>(
                rx->sctype_.elem_type(),
                make<Call_Expr>(state.ph(), to_expr(val,*lph), ix),
                state.cx)};
        }
    }
    throw Exception(state.cx, state.err(val, index, path+1, endpath));
}
Value value_at(Value list, Value index, Shared<const Phrase> callph, Frame& f)
{
    Index_State state(callph, f);
    // TODO: support reactive index
    auto path = index.to<List>(state.icx);
    return value_at_path(list, path->begin(), path->end(), state);
}
#endif

} // namespace curv
