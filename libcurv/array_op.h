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
            if (yre && yre->sctype_ == SC_Type::Num()) {
                auto& syn = f.cx.syntax();
                return {make<Reactive_Expression>(
                    SC_Type::Num(),
                    f.make_expr(
                        make<Constant>(share(syn), x),
                        yre->expr(syn)),
                    f.cx)};
            }
        }
        auto xre = x.dycast<Reactive_Value>();
        if (xre && xre->sctype_ == SC_Type::Num()) {
            auto& syn = f.cx.syntax();
            if (y.is_num())
                return {make<Reactive_Expression>(
                    SC_Type::Num(),
                    f.make_expr(xre->expr(syn),
                        make<Constant>(share(syn), y)),
                    f.cx)};
            auto yre = y.dycast<Reactive_Value>();
            if (yre && yre->sctype_ == SC_Type::Num())
                return {make<Reactive_Expression>(
                    SC_Type::Num(),
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

struct Function_Op
{
    At_Arg cx;
    Symbol_Ref name;
    Function_Op(Function& func, Frame& args)
    :
        cx(func, args),
        name(func.name_)
    {}
};

struct Binary_Boolean_Op : public Function_Op
{
    using Function_Op::Function_Op;
    typedef bool left_t, right_t;
    static bool unbox_left(Value a, left_t& b)
    {
        if (a.is_bool()) {
            b = a.get_bool_unsafe();
            return true;
        } else
            return false;
    }
    static bool unbox_right(Value a, right_t& b)
    {
        if (a.is_bool()) {
            b = a.get_bool_unsafe();
            return true;
        } else
            return false;
    }
};

// The left operand is a non-empty list of booleans.
// The right operand is an integer >= 0 and < the size of the left operand.
// (These restrictions on the right operand conform to the definition
// of << and >> in the C/C++/GLSL languages.)
struct Shift_Op : public Function_Op
{
    using Function_Op::Function_Op;
    typedef Shared<const List> left_t;
    typedef double right_t;
    static bool unbox_left(Value a, left_t& b)
    {
        b = a.dycast<const List>();
        return b && !b->empty() && b->front().is_bool();
    }
    static bool unbox_right(Value a, right_t& b)
    {
        b = a.get_num_or_nan();
        return b == b;
    }
};

template <class Scalar_Op>
struct Binary_Array_Op
{
    // TODO: optimize: move semantics. unique object reuse.
    // TODO: optimize: faster fast path in `op` for number case.

    static Shared<const String> domain_error(
        const Scalar_Op& f, Value x, Value y)
    {
        (void)f;
        return stringify("[",x,",",y,"]: domain error");
    }

    static Shared<const String> domain_error(
        const Scalar_Op& f, Value x)
    {
        (void)f;
        return stringify(x,": domain error");
    }

    static Value
    reduce(const Scalar_Op& f, Value zero, Value arg)
    {
        auto list = arg.to<List>(f.cx);
        Value result = zero;
        for (auto val : *list)
            result = op(f, result, val);
        return result;
    }

    static Value
    op(const Scalar_Op& f, Value x, Value y)
    {
        // fast path: both x and y are scalars
        // remaining cases:
        // - x is a scalar, y is a list
        // - x is a list, y is a scalar
        // - x and y are lists
        // - either x, or y, or both, is reactive
        typename Scalar_Op::left_t sx;
        typename Scalar_Op::right_t sy;
        if (f.unbox_left(x, sx)) {
            if (f.unbox_right(y, sy))
                return f.call(sx, sy);
            if (y.is_ref()) {
                Ref_Value& ry(y.get_ref_unsafe());
                switch (ry.type_) {
                case Ref_Value::ty_list:
                    return broadcast_right(f, x, (List&)ry);
                case Ref_Value::ty_reactive:
                    return reactive_op(f, x, y);
                }
            }
            throw Exception(At_Index(1,f.cx), domain_error(f, y));
        } else if (x.is_ref()) {
            Ref_Value& rx(x.get_ref_unsafe());
            switch (rx.type_) {
            case Ref_Value::ty_list:
                if (f.unbox_right(y, sy))
                    return broadcast_left(f, (List&)rx, y);
                else if (y.is_ref()) {
                    Ref_Value& ry(y.get_ref_unsafe());
                    switch (ry.type_) {
                    case Ref_Value::ty_list:
                        return element_wise_op(f, (List&)rx, (List&)ry);
                    case Ref_Value::ty_reactive:
                        return reactive_op(f, x, y);
                    }
                }
                throw Exception(At_Index(1,f.cx), domain_error(f, y));
            case Ref_Value::ty_reactive:
                return reactive_op(f, x, y);
            }
        }
        throw Exception(At_Index(0,f.cx), domain_error(f, x));
    }

    static Value
    broadcast_left(const Scalar_Op& f, List& xlist, Value y)
    {
        Shared<List> result = List::make(xlist.size());
        for (unsigned i = 0; i < xlist.size(); ++i)
            (*result)[i] = op(f, xlist[i], y);
        return {result};
    }

    static Value
    broadcast_right(const Scalar_Op& f, Value x, List& ylist)
    {
        Shared<List> result = List::make(ylist.size());
        for (unsigned i = 0; i < ylist.size(); ++i)
            (*result)[i] = op(f, x, ylist[i]);
        return {result};
    }

    static Value
    element_wise_op(const Scalar_Op& f, List& xs, List& ys)
    {
        if (xs.size() != ys.size())
            throw Exception(f.cx, stringify(
                f.name,
                ": mismatched list sizes (",
                xs.size(),",",ys.size(),") in array operation"));
        Shared<List> result = List::make(xs.size());
        for (unsigned i = 0; i < xs.size(); ++i)
            (*result)[i] = op(f, xs[i], ys[i]);
        return {result};
    }

    static Value
    reactive_op(const Scalar_Op& f, Value x, Value y)
    {
    /*
        // At least one of x and y is reactive. Cases:
        //   list * list
        //   scalar * list
        //   list * scalar
        //   scalar * scalar;
        // where list and scalar are either constants or reactive.
        // Need to compute the result SC_Type now, and thus I need to look at
        // these cases right here.
        Shared<const Reactive_Value> xr, yr;
        SC_Type xt, yt;
        f.get_expr(x, xr, xt);
        f.get_expr(y, yr, yt);
    //
        if (x.is_num()) {
            auto yre = y.dycast<Reactive_Value>();
            if (yre && yre->sctype_ == SC_Type::Num()) {
                auto& syn = f.cx.syntax();
                return {make<Reactive_Expression>(
                    SC_Type::Num(),
                    f.make_expr(
                        make<Constant>(share(syn), x),
                        yre->expr(syn)),
                    f.cx)};
            }
        }
    */
        throw Exception(f.cx, domain_error(f, x, y));
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
        if (xre && xre->sctype_ == SC_Type::Num()) {
            return {make<Reactive_Expression>(
                SC_Type::Num(),
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
