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

struct While_Indexing : public At_Syntax_Wrapper
{
    Value collection_;
    Value index_;
    While_Indexing(Value c, Value i, const At_Syntax& cx)
      : At_Syntax_Wrapper(cx), collection_(c), index_(i) {}
    Shared<const String> rewrite_message(Shared<const String> s) const override
    {
        return stringify(parent_.rewrite_message(s),
            "\ncollection: ", collection_,
            "\n@index: ", index_);
    }
};
struct Bad_Index : public At_Syntax_Wrapper
{
    Bad_Index(const At_Syntax& cx) : At_Syntax_Wrapper(cx) {}
    Shared<const String> rewrite_message(Shared<const String> s) const override
      { return stringify("Bad index: ", parent_.rewrite_message(s)); }
};
struct Bad_Collection : public At_Syntax_Wrapper
{
    Bad_Collection(const At_Syntax& cx) : At_Syntax_Wrapper(cx) {}
    Shared<const String> rewrite_message(Shared<const String> s) const override
      { return stringify("Index incompatible with collection: ",
            parent_.rewrite_message(s)); }
};

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
    const At_Syntax& gcx)
{
    While_Indexing lcx(value, index, gcx);
    if (index.is_num()) {
        double num = index.to_num_unsafe();
        if (num_is_int(num)) {
            Generic_List glist(value, Fail::hard, Bad_Collection(lcx));
            int i = num_to_int(index.to_num_unsafe(), 0, int(glist.size())-1,
                Bad_Index(lcx));
            auto r = glist.val_at(i, lcx);
            return get_value_at_slice(r, slice, endslice, gcx);
        }
    }
    else if (auto sym = maybe_symbol(index)) {
        auto rec = value.to<Record>(Bad_Collection(lcx));
        auto elem = rec->getfield(sym, Bad_Index(lcx));
        return get_value_at_slice(elem, slice, endslice, gcx);
    }
    else if (auto list = index.maybe<List>()) {
        List_Builder lb;
        for (auto i : *list) {
            auto r = get_value_at_index(value, i, slice, endslice, gcx);
            lb.push_back(r);
        }
        return lb.get_value();
    }
    else if (auto ri = index.maybe<Reactive_Value>()) {
        if (ri->sctype_.is_num()) {
            auto type = sc_type_of(value);
            if (type.is_list()) {
                auto r = make<Reactive_Expression>(
                    type.elem_type(),
                    make<Index_Expr>(
                        share(gcx.syntax()),
                        to_expr(value, index_value_phrase(gcx)),
                        ri->expr()),
                    gcx);
                return get_value_at_slice(Value{r}, slice, endslice, gcx);
            }
        } else if (ri->sctype_.is_list()) {
            // TODO
            // The only thing I need is the result type of value@index
            // and then it's the same code as above. Wait til SubCurv supports
            // this then use the function that computes the result type.
        }
    }
    throw Exception(lcx, stringify("Bad index: ", index));
}

} // namespace curv
