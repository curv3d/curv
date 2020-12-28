// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_LENS_H
#define LIBCURV_LENS_H

#include <libcurv/value.h>
#include <libcurv/context.h>

namespace curv {

struct IId : public Ref_Value
{
    IId() : Ref_Value(Ref_Value::ty_index, Ref_Value::sty_iid) {}
    virtual void print_repr(std::ostream&) const override;
};
struct IPath : public Ref_Value
{
    IPath(Value i1, Value i2)
    :
        Ref_Value(Ref_Value::ty_index, Ref_Value::sty_ipath),
        index1_(i1),
        index2_(i2)
    {}
    Value index1_, index2_;
    virtual void print_repr(std::ostream&) const override;
};
Value make_ipath(const Value* list, const Value* endlist);
struct ISlice : public Ref_Value
{
    ISlice(Value i1, Value i2)
    :
        Ref_Value(Ref_Value::ty_index, Ref_Value::sty_islice),
        index1_(i1),
        index2_(i2)
    {}
    Value index1_, index2_;
    virtual void print_repr(std::ostream&) const override;
};
Value make_islice(const Value* list, const Value* endlist);

// The 'slice' argument is unboxed to a list of index values.
Value get_value_at_boxed_slice(Value value, Value slice, const At_Syntax& cx);

Value index_fetch(Value tree, Value index, const At_Syntax& cx);
Value index_fetch_slice(Value tree, Value i1, Value i2, const At_Syntax& cx);
Value index_amend(Value tree, Value index, Value elems, const At_Syntax& cx);
Value index_amend_slice(Value tree, Value i1, Value i2, Value elems,
    const At_Syntax& cx);
Value index_over(Value tree, Value index,
    std::function<Value(Value, At_Syntax&)>, const At_Syntax& cx);

} // namespace curv
#endif // header guard
