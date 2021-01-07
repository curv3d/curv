// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/tree.h>

#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/list.h>
#include <libcurv/num.h>
#include <libcurv/reactive.h>
#include <libcurv/symbol.h>
#include <cmath>

namespace curv {

void TId::print_repr(std::ostream& out) const
{
    out << "tid";
}
void TSlice::print_repr(std::ostream& out) const
{
    out << "tslice [" << index1_ << "," << index2_ << "]";
}
void TPath::print_repr(std::ostream& out) const
{
    out << "tpath [" << index1_ << "," << index2_ << "]";
}
Value make_tpath(const Value* list, const Value* endlist)
{
    switch (endlist - list) {
    case 0:
        return Value{make<TId>()};
    case 1:
        return list[0];
    default:
        return Value{make<TPath>(list[0], make_tpath(list+1,endlist))};
    }
}
Value make_tslice(const Value* list, const Value* endlist)
{
    switch (endlist - list) {
    case 0:
        return Value{make<TId>()};
    case 1:
        return list[0];
    default:
        return Value{make<TSlice>(list[0], make_tslice(list+1,endlist))};
    }
}

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
            "\nindex: ", index_);
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

Value get_value_at_boxed_slice(Value value, Value slice, const At_Syntax& cx)
{
    auto list = slice.to<const List>(cx);
    return tree_fetch(value, make_tslice(list->begin(), list->end()), cx);
}

Value tree_fetch(Value tree, Value index, const At_Syntax& gcx)
{
    While_Indexing lcx(tree, index, gcx);
    if (index.is_num()) {
        double num = index.to_num_unsafe();
        if (num_is_int(num)) {
            Generic_List glist(tree, Fail::hard, Bad_Collection(lcx));
            int i = num_to_int(num, 0, int(glist.size())-1, Bad_Index(lcx));
            return glist.val_at(i, lcx);
        }
    }
    else if (auto sym = maybe_symbol(index)) {
        auto rec = tree.to<Record>(Bad_Collection(lcx));
        return rec->getfield(sym, Bad_Index(lcx));
    }
    else if (auto list = index.maybe<List>()) {
        List_Builder lb;
        for (auto i : *list) {
            auto r = tree_fetch(tree, i, gcx);
            lb.push_back(r);
        }
        return lb.get_value();
    }
    else if (auto path = index.maybe<TPath>()) {
        Value r = tree_fetch(tree, path->index1_, gcx);
        return tree_fetch(r, path->index2_, gcx);
    }
    else if (auto sli = index.maybe<TSlice>()) {
        return tree_fetch_slice(tree, sli->index1_, sli->index2_, gcx);
    }
    else if (auto id = index.maybe<TId>()) {
        return tree;
    }
    else if (auto ri = index.maybe<Reactive_Value>()) {
        if (ri->sctype_.is_num()) {
            auto type = sc_type_of(tree);
            if (type.is_list()) {
                return {make<Reactive_Expression>(
                    type.elem_type(),
                    make<Index_Expr>(
                        share(gcx.syntax()),
                        to_expr(tree, index_value_phrase(gcx)),
                        ri->expr()),
                    gcx)};
            }
        } else if (ri->sctype_.is_list()) {
            // TODO
            // The only thing I need is the result type of tree@index
            // and then it's the same code as above. Wait til SubCurv supports
            // this then use the function that computes the result type.
        }
    }
    throw Exception(lcx, stringify("Bad index: ", index));
}
Value tree_fetch_slice(Value tree, Value index, Value index2,
    const At_Syntax& gcx)
{
    While_Indexing lcx(tree, index, gcx);
    if (index.is_num()) {
        double num = index.to_num_unsafe();
        if (num_is_int(num)) {
            Generic_List glist(tree, Fail::hard, Bad_Collection(lcx));
            int i = num_to_int(num, 0, int(glist.size())-1, Bad_Index(lcx));
            auto r = glist.val_at(i, lcx);
            return tree_fetch(r, index2, gcx);
        }
    }
    else if (auto sym = maybe_symbol(index)) {
        auto rec = tree.to<Record>(Bad_Collection(lcx));
        auto elem = rec->getfield(sym, Bad_Index(lcx));
        return tree_fetch(elem, index2, gcx);
    }
    else if (auto list = index.maybe<List>()) {
        List_Builder lb;
        for (auto i : *list) {
            auto r = tree_fetch_slice(tree, i, index2, gcx);
            lb.push_back(r);
        }
        return lb.get_value();
    }
    else if (auto path = index.maybe<TPath>()) {
        Value r = tree_fetch(tree, path->index1_, gcx);
        return tree_fetch_slice(r, path->index2_, index2, gcx);
    }
    else if (auto slice = index.maybe<TSlice>()) {
        // Rewrite using the associative law of tslice[i,j].
        // This case normally doesn't happen, since islice[i1,i2,i3]
        // is normalized to islice[i1,islice[i2,i3]].
        return tree_fetch_slice(tree, slice->index1_,
            Value{make<TSlice>(slice->index2_, index2)},
            gcx);
    }
    else if (index.maybe<TId>()) {
        return tree_fetch(tree, index2, gcx);
    }
#if 0
    else if (auto ri = index.maybe<Reactive_Value>()) {
        if (ri->sctype_.is_num()) {
            auto type = sc_type_of(tree);
            if (type.is_list()) {
                auto r = make<Reactive_Expression>(
                    type.elem_type(),
                    make<Index_Expr>(
                        share(gcx.syntax()),
                        to_expr(value, index_value_phrase(gcx)),
                        ri->expr()),
                    gcx);
                return tree_fetch(Value{r}, slice, endslice, gcx);
            }
        } else if (ri->sctype_.is_list()) {
            // TODO
            // The only thing I need is the result type of value@index
            // and then it's the same code as above. Wait til SubCurv supports
            // this then use the function that computes the result type.
        }
    }
#endif
    throw Exception(lcx, stringify("Bad index: ", index));
}
Value tree_amend(Value tree, Value index, Value elems, const At_Syntax& gcx)
{
    While_Indexing lcx(tree, index, gcx);
    if (index.is_num()) {
        double num = index.to_num_unsafe();
        if (num_is_int(num)) {
            Generic_List glist(tree, Fail::hard, Bad_Collection(lcx));
            int i = num_to_int(num, 0, int(glist.size())-1, Bad_Index(lcx));
            glist.prepare_for_amend();
            glist.amend_at(i, elems, lcx);
            return glist.get_value();
        }
    }
    else if (auto sym = maybe_symbol(index)) {
        auto rec = update_drecord(std::move(tree), Bad_Collection(lcx));
        auto ref = rec->ref_field(sym, false, lcx);
        *ref = elems;
        return {rec};
    }
    else if (auto ilist = index.maybe<List>()) {
        Generic_List elist(elems, Fail::hard, lcx);
        ilist->assert_size(elist.size(), Bad_Index(lcx));
        auto r = tree;
        for (unsigned i = 0; i < elist.size(); ++i) {
            r = tree_amend(r, ilist->at(i), elist.val_at(i,lcx), gcx);
        }
        return r;
    }
    else if (auto path = index.maybe<TPath>()) {
        Value e = tree_fetch(tree, path->index1_, gcx);
        Value ne = tree_amend(e, path->index2_, elems, gcx);
        return tree_amend(tree, path->index1_, ne, gcx);
    }
    else if (auto sli = index.maybe<TSlice>()) {
        return tree_amend_slice(tree, sli->index1_, sli->index2_, elems, gcx);
    }
    else if (index.maybe<TId>()) {
        return elems;
    }
    // TODO: amend using a reactive index
    throw Exception(lcx, stringify("Bad index: ", index));
}
Value tree_amend_slice(Value tree, Value index, Value index2, Value elems,
    const At_Syntax& gcx)
{
    While_Indexing lcx(tree, index, gcx);
    if (index.is_num()) {
        double num = index.to_num_unsafe();
        if (num_is_int(num)) {
            Generic_List glist(tree, Fail::hard, Bad_Collection(lcx));
            int i = num_to_int(num, 0, int(glist.size())-1, Bad_Index(lcx));
            Value e = glist.val_at(i,lcx);
            Value ne = tree_amend(e, index2, elems, gcx);
            glist.prepare_for_amend();
            glist.amend_at(i, ne, lcx);
            return glist.get_value();
        }
    }
    else if (auto sym = maybe_symbol(index)) {
        auto rec = update_drecord(std::move(tree), Bad_Collection(lcx));
        auto ref = rec->ref_field(sym, false, lcx);
        Value ne = tree_amend(*ref, index2, elems, gcx);
        *ref = ne;
        return {rec};
    }
    else if (auto ilist = index.maybe<List>()) {
        Generic_List elist(elems, Fail::hard, lcx);
        ilist->assert_size(elist.size(), Bad_Index(lcx));
        auto r = tree;
        for (unsigned i = 0; i < elist.size(); ++i) {
            auto e = tree_fetch(r, ilist->at(i), gcx);
            auto ne = tree_amend(e, index2, elist.val_at(i,lcx), gcx);
            r = tree_amend(r, ilist->at(i), ne, gcx);
        }
        return r;
    }
    else if (auto path = index.maybe<TPath>()) {
        Value e = tree_fetch(tree, path->index1_, gcx);
        Value ne = tree_amend(e, path->index2_, elems, gcx);
        return tree_amend_slice(tree, path->index1_, index2, ne, gcx);
    }
    else if (auto slice = index.maybe<TSlice>()) {
        // Rewrite using the associative law of tslice[i,j].
        // This case is rare; only occurs with tslice[tslice[i,j],k].
        // Which usually doesn't happen since tslice[i,j,k]
        // is represented internally as tslice[i,tslice[j,k]].
        return tree_amend_slice(tree, slice->index1_,
            Value{make<TSlice>(slice->index2_, index2)},
            elems, gcx);
    }
    else if (index.maybe<TId>()) {
        return tree_amend(tree, index2, elems, gcx);
    }
    // TODO: amend using a reactive index
    throw Exception(lcx, stringify("Bad index: ", index));
}
Value tree_over(Value tree, Value index,
    std::function<Value(Value, At_Syntax&)> f, const At_Syntax& gcx)
{
    throw Exception(gcx, stringify("tree_over not implemented. index: ", index));
}

} // namespace curv
