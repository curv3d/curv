// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_TREE_H
#define LIBCURV_TREE_H

#include <libcurv/value.h>
#include <libcurv/context.h>

namespace curv {

struct This : public Ref_Value
{
    This() : Ref_Value(Ref_Value::ty_index, Ref_Value::sty_this) {}
    virtual void print_repr(std::ostream&) const override;
};
struct TPath : public Ref_Value
{
    TPath(Value i1, Value i2)
    :
        Ref_Value(Ref_Value::ty_index, Ref_Value::sty_tpath),
        index1_(i1),
        index2_(i2)
    {}
    Value index1_, index2_;
    virtual void print_repr(std::ostream&) const override;
    Ternary equal(const TPath& right, const Context& cx) const {
        return index1_.equal(right.index1_, cx)
             & index2_.equal(right.index2_, cx);
    }
};
Value make_tpath(const Value* list, const Value* endlist);
struct TSlice : public Ref_Value
{
    TSlice(Value i1, Value i2)
    :
        Ref_Value(Ref_Value::ty_index, Ref_Value::sty_tslice),
        index1_(i1),
        index2_(i2)
    {}
    Value index1_, index2_;
    virtual void print_repr(std::ostream&) const override;
    Ternary equal(const TSlice& right, const Context& cx) const {
        return index1_.equal(right.index1_, cx)
             & index2_.equal(right.index2_, cx);
    }
};
Value make_tslice(const Value* list, const Value* endlist);

// The 'slice' argument is unboxed to a list of index values.
Value get_value_at_boxed_slice(Value value, Value slice, const At_Syntax& cx);

Value tree_fetch(Value tree, Value index, const At_Syntax& cx);
Value tree_fetch_slice(Value tree, Value i1, Value i2, const At_Syntax& cx);
Value tree_amend(Value tree, Value index, Value elems, const At_Syntax& cx);
Value tree_amend_slice(Value tree, Value i1, Value i2, Value elems,
    const At_Syntax& cx);
Value tree_over(Value tree, Value index,
    std::function<Value(Value, At_Syntax&)>, const At_Syntax& cx);

} // namespace curv
#endif // header guard
