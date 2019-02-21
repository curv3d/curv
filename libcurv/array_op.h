// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_ARRAY_OP_H
#define LIBCURV_ARRAY_OP_H

// Curv is an array language, following APL and its successors.
// This means that scalar operations (on numbers and booleans)
// are generalized to operate on arrays of scalars, in two ways:
// element-wise operation, and broadcasting.
//   2 + 2 == 4                  -- a scalar operation
//   [3,4] + [10,20] == [13,24]  -- element-wise addition
//   1 + [10,20] == [11,21]      -- broadcasting

#include <libcurv/list.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/reactive.h>

namespace curv {

template <class Scalar_Op>
struct Binary_Numeric_Array_Op
{
    // TODO: optimize: move semantics. unique object reuse.

    static Value
    reduce(const Scalar_Op& f, double zero, Value arg)
    {
        auto list = arg.to<List>(f.cx);
        Value result = {zero};
        for (auto val : *list)
            result = op(f, result, val);
        return result;
    }

    static Value
    op(const Scalar_Op& f, Value x, Value y)
    {
        // if both x and y are numbers
        double r = f.call(x.get_num_or_nan(), y.get_num_or_nan());
        if (r == r)
            return {r};

        // if x, y, or both, are lists
        if (auto xlist = x.dycast<List>()) {
            if (auto ylist = y.dycast<List>())
                return {element_wise_op(f, xlist, ylist)};
            return {broadcast_left(f, xlist, y)};
        }
        if (auto ylist = y.dycast<List>())
            return {broadcast_right(f, x, ylist)};

        // One of x or y is reactive, the other is a number.
        // Both x and y are reactive.
        if (x.is_num()) {
            auto yre = y.dycast<Reactive_Value>();
            if (yre && yre->gltype_ == GL_Type::Num()) {
                auto& syn = f.cx.syntax();
                return {make<Reactive_Expression>(
                    GL_Type::Num(),
                    f.make_expr(
                        make<Constant>(share(syn), x),
                        yre->expr(syn)),
                    f.cx)};
            }
        }
        auto xre = x.dycast<Reactive_Value>();
        if (xre && xre->gltype_ == GL_Type::Num()) {
            auto& syn = f.cx.syntax();
            if (y.is_num())
                return {make<Reactive_Expression>(
                    GL_Type::Num(),
                    f.make_expr(xre->expr(syn),
                        make<Constant>(share(syn), y)),
                    f.cx)};
            auto yre = y.dycast<Reactive_Value>();
            if (yre && yre->gltype_ == GL_Type::Num())
                return {make<Reactive_Expression>(
                    GL_Type::Num(),
                    f.make_expr(xre->expr(syn), yre->expr(syn)),
                    f.cx)};
        }

        throw Exception(f.cx,
            stringify(f.callstr(x,y),": domain error"));
    }

    static Shared<List>
    broadcast_left(const Scalar_Op& f, Shared<List> xlist, Value y)
    {
        Shared<List> result = List::make(xlist->size());
        for (unsigned i = 0; i < xlist->size(); ++i)
            (*result)[i] = op(f, (*xlist)[i], y);
        return result;
    }

    static Shared<List>
    broadcast_right(const Scalar_Op& f, Value x, Shared<List> ylist)
    {
        Shared<List> result = List::make(ylist->size());
        for (unsigned i = 0; i < ylist->size(); ++i)
            (*result)[i] = op(f, x, (*ylist)[i]);
        return result;
    }

    static Shared<List>
    element_wise_op(const Scalar_Op& f, Shared<List> xs, Shared<List> ys)
    {
        if (xs->size() != ys->size())
            throw Exception(f.cx, stringify(
                f.name(),
                ": mismatched list sizes (",
                xs->size(),",",ys->size(),") in array operation"));
        Shared<List> result = List::make(xs->size());
        for (unsigned i = 0; i < xs->size(); ++i)
            (*result)[i] = op(f, (*xs)[i], (*ys)[i]);
        return result;
    }
};

template <class Scalar_Op>
struct Unary_Numeric_Array_Op
{
    // TODO: optimize: move semantics. unique object reuse.

    static Value
    op(const Scalar_Op& f, Value x)
    {
        double r = f.call(x.get_num_or_nan());
        if (r == r)
            return {r};
        if (auto xlist = x.dycast<List>())
            return {element_wise_op(f, xlist)};
        auto xre = x.dycast<Reactive_Value>();
        if (xre && xre->gltype_ == GL_Type::Num()) {
            return {make<Reactive_Expression>(
                GL_Type::Num(),
                f.make_expr(xre->expr(f.cx.syntax())),
                f.cx)};
        }
        throw Exception(f.cx,
            stringify(f.callstr(x),": domain error"));
    }

    static Shared<List>
    element_wise_op(const Scalar_Op& f, Shared<List> xs)
    {
        Shared<List> result = List::make(xs->size());
        for (unsigned i = 0; i < xs->size(); ++i)
            (*result)[i] = op(f, (*xs)[i]);
        return result;
    }
};

} // namespace
#endif // header guard
