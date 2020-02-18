// Copyright 2017-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/math.h>
#include <libcurv/context.h>
#include <libcurv/array_op.h>
#include <libcurv/reactive.h>

namespace curv {

bool isnum(Value a)
{
    if (a.is_num()) return true;
    auto r = a.dycast<Reactive_Value>();
    return r && r->sctype_ == SC_Type::Num();
}

bool isbool(Value a)
{
    if (a.is_bool()) return true;
    auto r = a.dycast<Reactive_Value>();
    return r && r->sctype_ == SC_Type::Bool();
}

bool issymbol(Value a)
{
    if (a.is_bool()) return true;
    if (a.dycast<Symbol>() != nullptr) return true;
    auto r = a.dycast<Reactive_Value>();
    return r && r->sctype_ == SC_Type::Bool();
}

bool islist(Value a)
{
    if (a.dycast<List>())
        return true;
    if (auto r = a.dycast<Reactive_Value>())
        return r->sctype_.is_list();
    return false;
}

Value multiply(Value a, Value b, const At_Syntax& cx)
{
#if 0
    struct Scalar_Op {
        static double call(double x, double y) { return x * y; }
        Shared<Operation> make_expr(
            Shared<Operation> x, Shared<Operation> y) const
        {
            return make<Multiply_Expr>(share(cx.syntax()),
                std::move(x), std::move(y));
        }
        static const char* name() { return "*"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify(x," * ",y);
        }
        const At_Syntax& cx;
        Scalar_Op(const At_Syntax& as) : cx(as) {}
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    return array_op.op(Scalar_Op(cx), a, b);
#else
    struct Mul_Prim : public Binary_Num_Prim
    {
        static const char* name() { return "*"; }
        static Value zero() { return {1.0}; }
        static Value call(double x, double y, const Context&)
            { return {x * y}; }
        static SC_Value sc_call(SC_Frame& f, SC_Value x, SC_Value y) {
            if (x.type.is_mat() && y.type.is_mat())
                return sc_bincall(f, x.type, "matrixCompMult", x, y);
            else
                return sc_binop(f, x.type, x, "*", y);
        }
    };
    using Mul_Op = Binary_Array_Op<Mul_Prim>;
    return Mul_Op::op(cx, a, b);
#endif
}

// Generalized dot product that includes vector dot product and matrix product.
// Same as Mathematica Dot[A,B]. Like APL A+.×B, Python numpy.dot(A,B)
//  dot(a,b) =
//    if (count a > 0 && is_list(a[0]))
//      [for (row in a) dot(row,b)]  // matrix*...
//    else
//      sum(a*b)                     // vector*...
Value dot(Value a, Value b, const At_Syntax& cx)
{
    auto av = a.to<List>(cx);
    auto bv = b.to<List>(cx);
    if (av->size() > 0 && av->at(0).dycast<List>()) {
        Shared<List> result = List::make(av->size());
        for (size_t i = 0; i < av->size(); ++i) {
            result->at(i) = dot(av->at(i), b, cx);
        }
        return {result};
    } else {
        if (av->size() != bv->size())
            throw Exception(cx, stringify("list of size ",av->size(),
                " can't be multiplied by list of size ",bv->size()));
        Value result = {0.0};
        for (size_t i = 0; i < av->size(); ++i)
            result = add(result, multiply(av->at(i), bv->at(i), cx), cx);
        return result;
    }
}

} // namespace curv
