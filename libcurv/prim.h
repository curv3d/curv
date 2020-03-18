// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_PRIM_H
#define LIBCURV_PRIM_H

// Curv is an array language, following APL and its successors.
// This means that scalar operations (on numbers and booleans)
// are generalized to operate on arrays of scalars, in two ways:
// element-wise operation, and broadcasting.
//   2 + 2 == 4                  -- a scalar operation
//   [3,4] + [10,20] == [13,24]  -- element-wise addition
//   1 + [10,20] == [11,21]      -- broadcasting

#include <libcurv/bool.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/list.h>
#include <libcurv/meaning.h>
#include <libcurv/num.h>
#include <libcurv/reactive.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/sc_context.h>
#include <libcurv/vec.h>

namespace curv {

//----------------------------------------------------------------------//
// Templates for converting an Array_Op to a unary or binary Expression //
//----------------------------------------------------------------------//

template <class Op>
struct Unary_Op_Expr : public Prefix_Expr_Base
{
    using Prefix_Expr_Base::Prefix_Expr_Base;
    virtual Value eval(Frame& f) const override
      { return Op::call(At_Phrase(*syntax_, f), arg_->eval(f)); }
    virtual SC_Value sc_eval(SC_Frame& f) const override
      { return Op::sc_op(At_SC_Phrase(syntax_,f), *arg_, f); }
    virtual void print(std::ostream& out) const override
      { out<<Op::Prim::name()<<"("<<*arg_<<")"; }
};

template <class Op>
struct Binary_Op_Expr : public Infix_Expr_Base
{
    using Infix_Expr_Base::Infix_Expr_Base;
    virtual Value eval(Frame& f) const override
      {return Op::call(At_Phrase(*syntax_, f), arg1_->eval(f), arg2_->eval(f));}
    virtual SC_Value sc_eval(SC_Frame& f) const override
      { return Op::sc_call(f, *arg1_, *arg2_, syntax_); }
    static bool idchr(char c)
        { return (c>='a'&&c<='z')||(c>='A'&&c<='Z')||c=='_'; }
    virtual void print(std::ostream& out) const override {
        if (idchr(Op::Prim::name()[0]))
            out<<Op::Prim::name()<<"["<<*arg1_<<","<<*arg2_<<"]";
        else
            out<<"("<<*arg1_<<Op::Prim::name()<<*arg2_<<")";
    }
};

//---------------------------------------------------------------//
// Templates for converting a Prim to a unary or binary Array_Op //
//---------------------------------------------------------------//

template <class PRIM>
struct Unary_Array_Op
{
    // TODO: optimize: move semantics. unique object reuse.

    using Prim = PRIM;

    static Value
    call(const At_Syntax& cx, Value x)
    {
        typename Prim::scalar_t sx;
        if (Prim::unbox(x, sx, cx)) {
            Value r = Prim::call(sx, cx);
            if (!r.is_missing()) return r;
        } else if (x.is_ref()) {
            Ref_Value& rx(x.to_ref_unsafe());
            switch (rx.type_) {
            case Ref_Value::ty_list:
                return element_wise_op(cx, (List&)rx);
            case Ref_Value::ty_reactive:
                return reactive_op(cx, (Reactive_Value&)rx);
            }
        }
        throw Exception(cx, domain_error(x));
    }
    static SC_Value
    sc_op(const At_Syntax& cx, Operation& argx, SC_Frame& f)
    {
        // TODO: add array support
        auto a = sc_eval_op(f, argx);
        Prim::sc_check_arg(a, cx);
        return Prim::sc_call(f, a);
    }

    static Value
    element_wise_op(const At_Syntax& cx, List& xs)
    {
        Shared<List> result = List::make(xs.size());
        for (unsigned i = 0; i < xs.size(); ++i)
            (*result)[i] = call(cx, xs[i]);
        return {result};
    }

    // Argument x is reactive. Construct a Reactive_Expression.
    static Value
    reactive_op(const At_Syntax& cx, Reactive_Value &rx)
    {
        SC_Type rtype = Prim::sc_result_type(rx.sctype_);
        if (rtype) {
            return {make<Reactive_Expression>(
                rtype,
                make<Unary_Op_Expr<Unary_Array_Op>>(
                    share(cx.syntax()), rx.expr()),
                cx)};
        } else {
            throw domain_error({share(rx)});
        }
    }

    static Shared<const String> domain_error(Value x)
    {
        return stringify(x, ": domain error");
    }
};

template <class PRIM>
struct Binary_Array_Op
{
    // TODO: optimize: move semantics. unique object reuse.
    // TODO: optimize: faster fast path in `op` for number case.

    using Prim = PRIM;

    static Exception domain_error(
        const At_Syntax& cx, unsigned i, Value x, Value y)
    {
        if (dynamic_cast<const At_Arg*>(&cx)) {
            return Exception(At_Index(i,cx),
                stringify(i==0?x:y,": domain error"));
        }
        if (auto ap = dynamic_cast<const At_Phrase*>(&cx)) {
            if (auto bin = dynamic_cast<const Binary_Phrase*>(&ap->phrase_))
                return Exception(cx,
                    stringify(x," ",bin->opname()," ",y,": domain error"));
        }
        return Exception(cx, stringify(i==0?x:y,": domain error"));
    }

    static Exception domain_error(
        const At_Syntax& cx, Value x, Value y)
    {
        if (auto ap = dynamic_cast<const At_Phrase*>(&cx)) {
            if (auto bin = dynamic_cast<const Binary_Phrase*>(&ap->phrase_))
                return Exception(cx,
                    stringify(x," ",bin->opname()," ",y,": domain error"));
        }
        return Exception(cx, stringify("[",x,",",y,"]: domain error"));
    }

    static Value
    reduce(const At_Syntax& cx, Value zero, Value arg)
    {
        auto list = arg.to<List>(cx);
        unsigned n = list->size();
        if (n == 0)
            return {zero};
        Value result = list->front();
        for (unsigned i = 1; i < n; ++i)
            result = call(cx, result, list->at(i));
        return result;
    }
    static SC_Value
    sc_reduce(const At_Syntax& cx, Value zero, Operation& argx, SC_Frame& f)
    {
        auto list = dynamic_cast<List_Expr*>(&argx);
        if (list) {
            if (list->empty())
                return sc_eval_const(f, zero, *argx.syntax_);
            auto first = sc_eval_op(f, *list->at(0));
            if (list->size() == 1) {
                Prim::sc_check_arg(first, cx);
                return first;
            }
            for (unsigned i = 1; i < list->size(); ++i) {
                auto second = sc_eval_op(f, *list->at(i));
                Prim::sc_check_args(f, first, second, cx);
                first = Prim::sc_call(f, first, second);
            }
            return first;
        }
        else {
            // Reduce an array value that exists at GPU run time.
            // TODO: For a large 1D array, use a GPU loop and call a function.
            // TODO: Binary_Array_Op::sc_reduce: reduce a matrix.
            // 2D arrays (SC_Type rank 2) are not supported, because you can't
            // generate a rank 1 array at GPU runtime, for now at least.
            // For a single Vec, this inline expansion of the loop is good.
            auto arg = sc_eval_op(f, argx);
            if (arg.type.is_any_vec()) {
                auto first = sc_vec_element(f, arg, 0);
                for (unsigned i = 1; i < arg.type.count(); ++i) {
                    auto second = sc_vec_element(f, arg, i);
                    Prim::sc_check_args(f, first, second, cx);
                    first = Prim::sc_call(f, first, second);
                }
                return first;
            }
            else {
                throw Exception(cx, "argument is not a vector");
            }
        }
    }

    static Value
    call(const At_Syntax& cx, Value x, Value y)
    {
        // fast path: both x and y are scalars
        // remaining cases:
        // - x is a scalar, y is a list
        // - x is a list, y is a scalar
        // - x and y are lists
        // - either x, or y, or both, is reactive
        typename Prim::left_t sx;
        typename Prim::right_t sy;
        if (Prim::unbox_left(x, sx, cx)) {
            if (Prim::unbox_right(y, sy, cx)) {
                Value r = Prim::call(sx, sy, cx);
                if (!r.is_missing()) return r;
            }
            else if (y.is_ref()) {
                Ref_Value& ry(y.to_ref_unsafe());
                switch (ry.type_) {
                case Ref_Value::ty_list:
                    return broadcast_right(cx, x, (List&)ry);
                case Ref_Value::ty_reactive:
                    return reactive_op(cx, x, y);
                }
            }
            throw domain_error(cx,x,y);
        } else if (x.is_ref()) {
            Ref_Value& rx(x.to_ref_unsafe());
            switch (rx.type_) {
            case Ref_Value::ty_list:
                if (Prim::unbox_right(y, sy, cx))
                    return broadcast_left(cx, (List&)rx, y);
                else if (y.is_ref()) {
                    Ref_Value& ry(y.to_ref_unsafe());
                    switch (ry.type_) {
                    case Ref_Value::ty_list:
                        return element_wise_op(cx, (List&)rx, (List&)ry);
                    case Ref_Value::ty_reactive:
                        return reactive_op(cx, x, y);
                    }
                }
                throw domain_error(cx,1,x,y);
            case Ref_Value::ty_reactive:
                return reactive_op(cx, x, y);
            }
        }
        throw domain_error(cx,0,x,y);
    }
    static SC_Value
    sc_op(const At_Syntax& cx, Operation& argx, SC_Frame& f)
    {
        auto list = dynamic_cast<List_Expr*>(&argx);
        if (list && list->size() == 2) {
            auto first = sc_eval_op(f, *list->at(0));
            auto second = sc_eval_op(f, *list->at(1));
            Prim::sc_check_args(f, first, second, cx);
            return Prim::sc_call(f, first, second);
        }
        // TODO: Binary_Array_Op::sc_op: accept a 2-vector, 2-array or mat2.
        throw Exception(cx, "expected a list of size 2");
    }
    static SC_Value
    sc_call(SC_Frame& f, Operation& ax, Operation& ay, Shared<const Phrase> ph)
    {
        auto x = sc_eval_op(f, ax);
        auto y = sc_eval_op(f, ay);
        Prim::sc_check_args(f, x, y, At_SC_Phrase(ph, f));
        return Prim::sc_call(f, x, y);
    }

    static Value
    broadcast_left(const At_Syntax& cx, List& xlist, Value y)
    {
        Shared<List> result = List::make(xlist.size());
        for (unsigned i = 0; i < xlist.size(); ++i)
            (*result)[i] = call(cx, xlist[i], y);
        return {result};
    }

    static Value
    broadcast_right(const At_Syntax& cx, Value x, List& ylist)
    {
        Shared<List> result = List::make(ylist.size());
        for (unsigned i = 0; i < ylist.size(); ++i)
            (*result)[i] = call(cx, x, ylist[i]);
        return {result};
    }

    static Value
    element_wise_op(const At_Syntax& cx, List& xs, List& ys)
    {
        if (xs.size() != ys.size())
            throw Exception(cx, stringify(
                "mismatched list sizes (",
                xs.size(),",",ys.size(),") in array operation"));
        Shared<List> result = List::make(xs.size());
        for (unsigned i = 0; i < xs.size(); ++i)
            (*result)[i] = call(cx, xs[i], ys[i]);
        return {result};
    }

    // At least one of x and y is reactive. Construct a Reactive_Expression.
    static Value
    reactive_op(const At_Syntax& cx, Value x, Value y)
    {
        Shared<Operation> x_expr;
        SC_Type x_type;
        if (auto xr = x.maybe<Reactive_Value>()) {
            x_expr = xr->expr();
            x_type = xr->sctype_;
        } else {
            x_expr = make<Constant>(share(cx.syntax()), x);
            x_type = sc_type_of(x);
        }

        Shared<Operation> y_expr;
        SC_Type y_type;
        if (auto yr = y.maybe<Reactive_Value>()) {
            y_expr = yr->expr();
            y_type = yr->sctype_;
        } else {
            y_expr = make<Constant>(share(cx.syntax()), y);
            y_type = sc_type_of(y);
        }

        SC_Type rtype = Prim::sc_result_type(x_type, y_type);
        if (rtype) {
            return {make<Reactive_Expression>(
                rtype,
                make<Binary_Op_Expr<Binary_Array_Op>>(
                    share(cx.syntax()), x_expr, y_expr),
                cx)};
        } else {
            throw domain_error(cx, x, y);
        }
    }
};

//----------------------------------------------------------------------------//
// Base types for Prim classes.                                               //
// Each Prim class defines the semantics of a primitive operator or function. //
// Each base class defines the argument and result types of a set of Prims.   //
//----------------------------------------------------------------------------//

// This Prim accepts arbitrary non-List arguments.
struct Unary_Scalar_Prim
{
    typedef Value scalar_t;
    static bool unbox(Value a, scalar_t& b, const Context&)
    {
        if (a.maybe<List>() || a.maybe<Reactive_Value>) {
            return false;
        } else {
            b = a;
            return true;
        }
    }
    static void sc_check_arg(SC_Value a, const Context& cx)
    {
        if (!a.type.is_struc())
            throw Exception(cx, "argument must be a Struc");
    }
};

// A primitive that maps [scalar,scalar] -> Bool
// In SubCurv, each argument is a scalar or vec[2-4].
struct Binary_Scalar_To_Bool_Prim : public Unary_Scalar_Prim
{
    typedef Value left_t, right_t;
    static bool unbox_left(Value a, left_t& b, const Context& cx)
    {
        return unbox(a, b, cx);
    }
    static bool unbox_right(Value a, right_t& b, const Context& cx)
    {
        return unbox(a, b, cx);
    }
    static void sc_check_args(
        SC_Frame& f, SC_Value& a, SC_Value& b, const Context& cx)
    {
        if (!a.type.is_scalar_or_vec()) {
            throw Exception(cx, stringify(
                "first argument must be a scalar or vec; instead got ",
                a.type));
        }
        if (!b.type.is_scalar_or_vec()) {
            throw Exception(cx, stringify(
                "second argument must be a scalar or vec; instead got ",
                b.type));
        }
        sc_struc_unify(f, a, b, cx);
    }
    static SC_Type sc_result_type(SC_Type a, SC_Type b)
    {
        SC_Type r = sc_unify_tensor_types(a, b);
        if (r && r.is_scalar_or_vec())
            return SC_Type::Bool(r.count());
        else
            return {};
    }
};

// A primitive mapping Bool -> Num.
// In SubCurv, argument types are Bool or Bvec but not Bool32.
struct Unary_Bool_To_Num_Prim
{
    typedef bool scalar_t;
    static bool unbox(Value a, scalar_t& b, const Context& cx)
    {
        if (a.is_bool()) {
            b = a.to_bool_unsafe();
            return true;
        } else
            return false;
    }
    static void sc_check_arg(SC_Value a, const Context& cx)
    {
        if (a.type.is_bool_or_vec()) return;
        throw Exception(cx, "argument must be Bool or BVec");
    }
    static SC_Type sc_result_type(SC_Type a)
    {
        if (a.is_bool_or_vec())
            return SC_Type::Num_Or_Vec(a.count());
        else
            return {};
    }
};

// maps bool -> bool
struct Unary_Bool_Prim
{
    typedef bool scalar_t;
    static bool unbox(Value a, scalar_t& b, const Context& cx)
    {
        if (a.is_bool()) {
            b = a.to_bool_unsafe();
            return true;
        } else
            return false;
    }
    static void sc_check_arg(SC_Value a, const Context& cx)
    {
        if (a.type.is_bool_struc()) return;
        throw Exception(cx, stringify("expected Bool or Bool32, got ",a.type));
    }
    static SC_Type sc_result_type(SC_Type a)
    {
        if (a.is_bool_struc())
            return a;
        else
            return {};
    }
};

// Maps [bool,bool] -> bool.
// In SubCurv, accepts Bool and Bool32 arguments, and vec of same.
struct Binary_Bool_Prim
{
    typedef bool left_t, right_t;
    static bool unbox_left(Value a, left_t& b, const Context&)
    {
        if (a.is_bool()) {
            b = a.to_bool_unsafe();
            return true;
        } else
            return false;
    }
    static bool unbox_right(Value a, right_t& b, const Context&)
    {
        if (a.is_bool()) {
            b = a.to_bool_unsafe();
            return true;
        } else
            return false;
    }
    static void sc_check_arg(SC_Value a, const Context& cx)
    {
        if (a.type.is_bool_struc()) return;
        throw Exception(cx, "argument must be Bool or Bool32");
    }
    static void sc_check_args(
        SC_Frame& /*f*/, SC_Value& a, SC_Value& b, const Context& cx)
    {
        if (a.type.is_bool_or_vec()) {
            if (b.type.is_bool_or_vec()) {
                if (a.type.count() != b.type.count()
                    && a.type.count() > 1 && b.type.count() > 1)
                {
                    throw Exception(cx, stringify(
                        "can't combine lists of different sizes (",
                        a.type.count(), " and ", b.type.count(), ")"));
                }
                return;
            }
            else if (b.type.is_bool32_or_vec()) {
                // TODO: convert a to Bool32 via broadcasting
            }
        }
        else if (a.type.is_bool32_or_vec()) {
            if (b.type.is_bool32_or_vec()) {
                if (a.type.count() != b.type.count()
                    && a.type.count() > 1 && b.type.count() > 1)
                {
                    throw Exception(cx, stringify(
                        "can't combine lists of different sizes (",
                        a.type.count(), " and ", b.type.count(), ")"));
                }
                return;
            }
            if (b.type.is_bool()) {
                // TODO: convert b to Bool32 via broadcasting?
            }
        }
        throw Exception(cx, stringify(
            "arguments must be Bool or Bool32 (got ",
            a.type, " and ", b.type, " instead)"));
    }
    static SC_Type sc_result_type(SC_Type a, SC_Type b)
    {
        if (a.is_bool_struc() && b.is_bool_struc())
            return sc_unify_tensor_types(a,b);
        else
            return {};
    }
};

// A primitive mapping number -> number.
// The corresponding GLSL primitive accepts a number, vector or matrix.
struct Unary_Num_SCMat_Prim
{
    typedef double scalar_t;
    static bool unbox(Value a, scalar_t& b, const Context&)
    {
        if (a.is_num()) {
            b = a.to_num_unsafe();
            return true;
        } else
            return false;
    }
    static void sc_check_arg(SC_Value a, const Context& cx)
    {
        if (!a.type.is_num_struc())
            throw Exception(cx, "argument must be a Num, Vec or Mat");
    }
    static SC_Type sc_result_type(SC_Type a)
    {
        return a.is_num_struc() ? a : SC_Type{};
    }
};

// A primitive mapping number -> number.
// The corresponding GLSL primitive accepts a number or vector.
struct Unary_Num_SCVec_Prim : public Unary_Num_SCMat_Prim
{
    static void sc_check_arg(SC_Value a, const Context& cx)
    {
        if (!a.type.is_num_or_vec())
            throw Exception(cx, "argument must be a Num or Vec");
    }
    static SC_Type sc_result_type(SC_Type a)
    {
        return a.is_num_or_vec() ? a : SC_Type{};
    }
};

// A primitive mapping number -> bool32.
// The corresponding GLSL primitive accepts a number or vector.
struct Unary_Num_To_Bool32_Prim : public Unary_Num_SCVec_Prim
{
    static SC_Type sc_result_type(SC_Type a)
    {
        return a.is_num_or_vec() ? SC_Type::Bool32(a.count()) : SC_Type{};
    }
};

// Maps [num,num] -> num.
// The corresponding GLSL primitive accepts number, vector or matrix arguments.
struct Binary_Num_SCMat_Prim : public Unary_Num_SCMat_Prim
{
    typedef double left_t;
    typedef double right_t;
    static bool unbox_left(Value a, scalar_t& b, const Context& cx)
    {
        return unbox(a, b, cx);
    }
    static bool unbox_right(Value a, scalar_t& b, const Context& cx)
    {
        return unbox(a, b, cx);
    }
    static void sc_check_args(
        SC_Frame& f, SC_Value& a, SC_Value& b, const Context& cx)
    {
        if (!a.type.is_num_struc()) {
            throw Exception(At_Index(0, cx),
                stringify("argument expected to be Num, Vec or Mat; got ",
                    a.type));
        }
        if (!b.type.is_num_struc()) {
            throw Exception(At_Index(1, cx),
                stringify("argument expected to be Num, Vec or Mat; got ",
                    a.type));
        }
        sc_struc_unify(f, a, b, cx);
    }
    static SC_Type sc_result_type(SC_Type a, SC_Type b)
    {
        if (a.is_num_tensor() && b.is_num_tensor())
            return sc_unify_tensor_types(a, b);
        else
            return {};
    }
};

// Maps [Num,Num] -> Num.
// The corresponding GLSL primitive accepts number or vector arguments.
struct Binary_Num_SCVec_Prim : public Unary_Num_SCVec_Prim
{
    typedef double left_t;
    typedef double right_t;
    static bool unbox_left(Value a, scalar_t& b, const Context& cx)
    {
        return unbox(a, b, cx);
    }
    static bool unbox_right(Value a, scalar_t& b, const Context& cx)
    {
        return unbox(a, b, cx);
    }
    static void sc_check_args(
        SC_Frame& f, SC_Value& a, SC_Value& b, const Context& cx)
    {
        if (!a.type.is_num_or_vec()) {
            throw Exception(At_Index(0, cx),
                stringify("argument expected to be Num or Vec; got ", a.type));
        }
        if (!b.type.is_num_or_vec()) {
            throw Exception(At_Index(1, cx),
                stringify("argument expected to be Num or Vec; got ", a.type));
        }
        sc_struc_unify(f, a, b, cx);
    }
    static SC_Type sc_result_type(SC_Type a, SC_Type b)
    {
        if (a.is_num_tensor() && b.is_num_tensor())
            return sc_unify_tensor_types(a, b);
        else
            return {};
    }
};

// maps [num,num] -> bool
struct Binary_Num_To_Bool_Prim : public Binary_Num_SCVec_Prim
{
    static SC_Type sc_result_type(SC_Type a, SC_Type b)
    {
        if (a.is_num_or_vec() && b.is_num_or_vec()) {
            SC_Type r = sc_unify_tensor_types(a,b);
            if (r) return SC_Type::Bool(r.count());
        }
        return {};
    }
};

// The left operand is a non-empty list of booleans.
// The right operand is an integer >= 0 and < the size of the left operand.
// The result has the same type as the left operand.
// (These restrictions on the right operand conform to the definition
// of << and >> in the C/C++/GLSL languages.)
struct Shift_Prim
{
    typedef Shared<const List> left_t;
    typedef double right_t;
    static bool unbox_left(Value a, left_t& b, const Context&)
    {
        b = a.maybe<const List>();
        return b && !b->empty() && b->front().is_bool();
    }
    static bool unbox_right(Value a, right_t& b, const Context&)
    {
        b = a.to_num_or_nan();
        return b == b;
    }
    static void sc_check_args(
        SC_Frame& /*f*/, SC_Value& a, SC_Value& b, const Context& cx)
    {
        if (!a.type.is_bool32_or_vec()) {
            throw Exception(At_Index(0, cx),
                stringify("expected argument of type Bool32, got ", a.type));
        }
        if (b.type != SC_Type::Num()) {
            throw Exception(At_Index(1, cx),
                stringify("expected argument of type Num, got ", b.type));
        }
    }
    static SC_Type sc_result_type(SC_Type a, SC_Type b)
    {
        if (a.is_bool32_or_vec() && b.is_num())
            return a;
        else
            return {};
    }
};

struct Unary_Vec2_To_Num_Prim
{
    typedef Vec2 scalar_t;
    static bool unbox(Value a, scalar_t& b, const Context&)
        { return unbox_vec2(a, b); }
    static void sc_check_arg(SC_Value a, const Context& cx)
    {
        if (a.type != SC_Type::Vec(2)) {
            throw Exception(cx, stringify("expected a Vec2; got ", a.type));
        }
    }
    static SC_Type sc_result_type(SC_Type a)
    {
        if (a == SC_Type::Vec(2))
            return SC_Type::Num();
        else
            return {};
    }
};

struct Bool32_Prim
{
    static bool unbox_bool32(Value in, unsigned& out, const Context& cx)
    {
        auto li = in.maybe<const List>();
        if (!li || li->size() != 32 || !li->front().is_bool())
            return false;
        out = bool32_to_nat(li, cx);
        return true;
    }
};
struct Unary_Bool32_To_Num_Prim : public Bool32_Prim
{
    typedef unsigned scalar_t;
    static bool unbox(Value a, scalar_t& b, const Context& cx)
    {
        return unbox_bool32(a, b, cx);
    }
    static void sc_check_arg(SC_Value a, const Context& cx)
    {
        if (!a.type.is_bool32_or_vec())
            throw Exception(cx, "argument must be a Bool32 or list of Bool32");
    }
    static SC_Type sc_result_type(SC_Type a)
    {
        if (a.is_bool32_or_vec())
            return SC_Type::Num_Or_Vec(a.count());
        else
            return {};
    }
};
// maps [bool32,bool32] -> bool32
struct Binary_Bool32_Prim : public Bool32_Prim
{
    typedef unsigned left_t;
    typedef unsigned right_t;
    static bool unbox_left(Value a, left_t& b, const Context& cx)
    {
        return unbox_bool32(a, b, At_Index(0, cx));
    }
    static bool unbox_right(Value a, right_t& b, const Context& cx)
    {
        return unbox_bool32(a, b, At_Index(1, cx));
    }
    static void sc_check_arg(SC_Value a, const Context& cx)
    {
        if (!a.type.is_bool32_or_vec())
            throw Exception(cx, "argument must be a Bool32 or list of Bool32");
    }
    static void sc_check_args(
        SC_Frame& /*f*/, SC_Value& a, SC_Value& b, const Context& cx)
    {
        if (!a.type.is_bool32_or_vec()) {
            throw Exception(At_Index(0, cx),
                stringify("expected argument of type Bool32, got ", a.type));
        }
        if (!b.type.is_bool32_or_vec()) {
            throw Exception(At_Index(1, cx),
                stringify("expected argument of type Bool32, got ", b.type));
        }
        if (a.type != b.type) {
            // Note, it's impossible to unify types of a and b
            // with the current palette of struc types.
            throw Exception(cx,
                stringify("mismatched argument types ",a.type," and ",b.type));
        }
    }
    static SC_Type sc_result_type(SC_Type a, SC_Type b)
    {
        if (a.is_bool32_or_vec() && b.is_bool32_or_vec() && a == b)
            return a;
        else
            return {};
    }
};

} // namespace
#endif // header guard
