// Copyright 2016-2018 Doug Moen
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

namespace curv {

template <class Scalar_Op>
struct Binary_Numeric_Array_Op
{
    // TODO: optimize: move semantics. unique object reuse.

    static Value
    reduce(double zero, Value arg, const At_Syntax& cx)
    {
        auto list = arg.to<List>(cx);
        Value result = {zero};
        for (auto val : *list)
            result = op(result, val, cx);
        return result;
    }

    static Value
    op(Value x, Value y, const At_Syntax& cx)
    {
        double r = Scalar_Op::f(x.get_num_or_nan(), y.get_num_or_nan());
        if (r == r)
            return {r};
        if (auto xlist = x.dycast<List>()) {
            if (auto ylist = y.dycast<List>())
                return {element_wise_op(xlist, ylist, cx)};
            return {broadcast_left(xlist, y, cx)};
        }
        if (auto ylist = y.dycast<List>())
            return {broadcast_right(x, ylist, cx)};
        throw Exception(cx,
            stringify(Scalar_Op::callstr(x,y),": domain error"));
    }

    static Shared<List>
    broadcast_left(Shared<List> xlist, Value y, const At_Syntax& cx)
    {
        Shared<List> result = List::make(xlist->size());
        for (unsigned i = 0; i < xlist->size(); ++i) {
            Value ex = (*xlist)[i];
            double r = Scalar_Op::f(ex.get_num_or_nan(),y.get_num_or_nan());
            if (r == r)
                (*result)[i] = {r};
            else if (auto exlist = ex.dycast<List>())
                (*result)[i] = {broadcast_left(exlist, y, cx)};
            else
                throw Exception(cx,
                    stringify(Scalar_Op::callstr(ex,y),": domain error"));
        }
        return result;
    }

    static Shared<List>
    broadcast_right(Value x, Shared<List> ylist, const At_Syntax& cx)
    {
        Shared<List> result = List::make(ylist->size());
        for (unsigned i = 0; i < ylist->size(); ++i) {
            Value ey = (*ylist)[i];
            double r = Scalar_Op::f(x.get_num_or_nan(), ey.get_num_or_nan());
            if (r == r)
                (*result)[i] = {r};
            else if (auto eylist = ey.dycast<List>())
                (*result)[i] = {broadcast_right(x, eylist, cx)};
            else
                throw Exception(cx,
                    stringify(Scalar_Op::callstr(x,ey),": domain error"));
        }
        return result;
    }

    static Shared<List>
    element_wise_op(Shared<List> xs, Shared<List> ys, const At_Syntax& cx)
    {
        if (xs->size() != ys->size())
            throw Exception(cx, stringify(
                Scalar_Op::name(),
                ": mismatched list sizes (",
                xs->size(),",",ys->size(),") in array operation"));
        Shared<List> result = List::make(xs->size());
        for (unsigned i = 0; i < xs->size(); ++i)
            (*result)[i] = op((*xs)[i], (*ys)[i], cx);
        return result;
    }
};

template <class Scalar_Op>
struct Unary_Numeric_Array_Op
{
    // TODO: optimize: move semantics. unique object reuse.

    static Value
    op(Value x, const Context& cx)
    {
        double r = Scalar_Op::f(x.get_num_or_nan());
        if (r == r)
            return {r};
        if (auto xlist = x.dycast<List>())
            return {element_wise_op(xlist, cx)};
        throw Exception(cx,
            stringify(Scalar_Op::callstr(x),": domain error"));
    }

    static Shared<List>
    element_wise_op(Shared<List> xs, const Context& cx)
    {
        Shared<List> result = List::make(xs->size());
        for (unsigned i = 0; i < xs->size(); ++i)
            (*result)[i] = op((*xs)[i], cx);
        return result;
    }
};

} // namespace
#endif // header guard
