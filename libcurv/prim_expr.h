// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_PRIM_EXPR_H
#define LIBCURV_PRIM_EXPR_H

// Curv primitive expressions which are implemented using the Prim framework.

#include <libcurv/prim.h>
#include <cmath>

namespace curv {

struct Positive_Prim : public Unary_Num_SCMat_Prim
{
    static const char* name() {return "+";};
    static Value call(double x, const Context&) { return {+x}; }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x)
        { return sc_unary_call(fm, x.type, "+", x); }
};
using Positive_Op = Unary_Array_Op<Positive_Prim>;
using Positive_Expr = Unary_Op_Expr<Positive_Op>;

struct Negative_Prim : public Unary_Num_SCMat_Prim
{
    static const char* name() {return "-";};
    static Value call(double x, const Context&) { return {-x}; }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x)
        { return sc_unary_call(fm, x.type, "-", x); }
};
using Negative_Op = Unary_Array_Op<Negative_Prim>;
using Negative_Expr = Unary_Op_Expr<Negative_Op>;

struct Not_Prim : public Unary_Bool_Prim
{
    static const char* name() {return "not";};
    static Value call(bool x, const Context&) { return {!x}; }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x) {
        if (x.type.is_bool())
            return sc_unary_call(fm, x.type, "!", x);
        if (x.type.is_bool_or_vec()) {
            if (fm.sc_.target_ == SC_Target::cpp)
                return sc_unary_call(fm, x.type, "not_", x);
            else
                return sc_unary_call(fm, x.type, "not", x);
        }
        return sc_unary_call(fm, x.type, "~", x);
    }
};
using Not_Op = Unary_Array_Op<Not_Prim>;
using Not_Expr = Unary_Op_Expr<Not_Op>;

// a++b catenates two lists or strings
struct Cat_Op : public Binary_Op
{
    static Value call(Fail fl, const Context& cx, Value a, Value b)
    {
        (void)fl; // TODO
        List_Builder lb;
        lb.concat(a, cx);
        lb.concat(b, cx);
        return lb.get_value();
    }
    struct Prim {
        static const char* name() {return "++";};
    };
};
using Cat_Expr = Binary_Op_Expr<Cat_Op>;

struct Add_Prim : public Binary_Num_SCMat_Prim
{
    static const char* name() {return "+";};
    static Value zero() { return {0.0}; }
    static Value call(double x, double y, const Context&) { return {x + y}; }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y)
        { return sc_binop(fm, x.type, x, "+", y); }
};
using Add_Op = Binary_Array_Op<Add_Prim>;
using Add_Expr = Binary_Op_Expr<Add_Op>;

struct Multiply_Prim : public Binary_Num_SCMat_Prim
{
    static const char* name() { return "*"; }
    static Value zero() { return {1.0}; }
    static Value call(double x, double y, const Context&) { return {x * y}; }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y) {
        if (x.type.is_mat() && y.type.is_mat())
            return sc_bincall(fm, x.type, "matrixCompMult", x, y);
        else
            return sc_binop(fm, x.type, x, "*", y);
    }
};
using Multiply_Op = Binary_Array_Op<Multiply_Prim>;
using Multiply_Expr = Binary_Op_Expr<Multiply_Op>;

struct Subtract_Prim : public Binary_Num_SCMat_Prim
{
    static const char* name() {return "-";};
    static Value call(double x, double y, const Context&) { return {x - y}; }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y)
        { return sc_binop(fm, x.type, x, "-", y); }
};
using Subtract_Op = Binary_Array_Op<Subtract_Prim>;
using Subtract_Expr = Binary_Op_Expr<Subtract_Op>;

struct Divide_Prim : public Binary_Num_SCMat_Prim
{
    static const char* name() {return "/";};
    static Value call(double x, double y, const Context&) { return {x / y}; }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y)
        { return sc_binop(fm, x.type, x, "/", y); }
};
using Divide_Op = Binary_Array_Op<Divide_Prim>;
using Divide_Expr = Binary_Op_Expr<Divide_Op>;

struct Power_Prim : public Binary_Num_SCVec_Prim
{
    static const char* name() {return "^";};
    static Value call(double x, double y, const Context&) { return {pow(x,y)}; }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y)
        { return sc_bincall(fm, x.type, "pow", x, y); }
};
using Power_Op = Binary_Array_Op<Power_Prim>;
using Power_Expr = Binary_Op_Expr<Power_Op>;

#define RELATION(Class,LT,lessThan) \
struct Class##_Prim : public Binary_Num_To_Bool_Prim \
{ \
    static const char* name() { return #LT; } \
    static Value call(double a, double b, const Context& cx) \
        { return {a LT b}; } \
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y) \
    { \
        if (x.type.is_num()) \
            return sc_binop(fm, SC_Type::Bool(), x, #LT, y); \
        else \
            return sc_bincall(fm,SC_Type::Bool(x.type.count()),#lessThan,x,y); \
    } \
}; \
using Class##_Op = Binary_Array_Op<Class##_Prim>; \
using Class##_Expr = Binary_Op_Expr<Class##_Op>;
RELATION(Less, <, lessThan)
RELATION(Greater, >, greaterThan)
RELATION(Less_Or_Equal, <=, lessThanEqual)
RELATION(Greater_Or_Equal, >=, greaterThanEqual)
#undef RELATION

} // namespace
#endif // header guard
