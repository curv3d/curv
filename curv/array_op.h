// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_ARRAY_OP_H
#define CURV_ARRAY_OP_H

// Curv is an array language, following APL and its successors.
// This means that scalar operations (on numbers and booleans)
// are generalized to operate on arrays of scalars, in two ways:
// element-wise operation, and broadcasting.
//   2 + 2 == 4                  -- a scalar operation
//   [3,4] + [10,20] == [13,24]  -- element-wise addition
//   1 + [10,20] == [11,21]      -- broadcasting

#include <curv/list.h>
#include <curv/context.h>
#include <curv/exception.h>
#include <curv/arg.h>

namespace curv {

template <class Scalar_Op>
struct Binary_Numeric_Array_Op
{
    // TODO: optimize: move semantics. unique object reuse.

    static Value
    reduce(double zero, Value arg, const Context& cx)
    {
        auto& list = arg_to_list(arg, cx);
        Value result = {zero};
        for (auto val : list)
            result = op(result, val, cx);
        return result;
    }

    static Value
    op(Value x, Value y, const Context& cx)
    {
        if (x.is_num()) {
            double xnum = x.get_num_unsafe();
            if (y.is_num())
                return {Scalar_Op::f(xnum, y.get_num_unsafe())};
            if (auto ylist = y.dycast<List>())
                return {broadcast_op(xnum, ylist, cx)};
        } else if (auto xlist = x.dycast<List>()) {
            if (y.is_num())
                return {broadcast_op(y.get_num_unsafe(), xlist, cx)};
            if (auto ylist = y.dycast<List>())
                return {element_wise_op(xlist, ylist, cx)};
        }
        throw Exception(cx, "domain error");
    }

    static Shared<List>
    broadcast_op(double xnum, Shared<List> ylist, const Context &cx)
    {
        Shared<List> result = List::make(ylist->size());
        for (unsigned i = 0; i < ylist->size(); ++i) {
            Value e = (*ylist)[i];
            if (e.is_num())
                (*result)[i] = {Scalar_Op::f(xnum, e.get_num_unsafe())};
            else if (auto elist = e.dycast<List>())
                (*result)[i] = {broadcast_op(xnum, elist, cx)};
            else
                throw Exception(cx, "domain error");
        }
        return result;
    }

    static Shared<List>
    element_wise_op(Shared<List> xs, Shared<List> ys, const Context& cx)
    {
        if (xs->size() != ys->size())
            throw Exception(cx, "mismatched list sizes in array operation");
        Shared<List> result = List::make(xs->size());
        for (unsigned i = 0; i < xs->size(); ++i)
            (*result)[i] = op((*xs)[i], (*ys)[i], cx);
        return result;
    }
};

} // namespace
#endif // header guard
