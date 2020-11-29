// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/builtin.h>

#include <libcurv/analyser.h>
#include <libcurv/bool.h>
#include <libcurv/die.h>
#include <libcurv/dir_record.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/import.h>
#include <libcurv/num.h>
#include <libcurv/pattern.h>
#include <libcurv/picker.h>
#include <libcurv/prim_expr.h>
#include <libcurv/program.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/sc_context.h>
#include <libcurv/source.h>
#include <libcurv/system.h>

#include <boost/math/constants/constants.hpp>
#include <boost/filesystem.hpp>

#include <cassert>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <string>

using namespace std;
using namespace boost::math::double_constants;

namespace curv {

Shared<Meaning>
Builtin_Value::to_meaning(const Identifier& id) const
{
    return make<Constant>(share(id), value_);
}

//----------------------------------------------//
// Templates for constructing builtin functions //
//----------------------------------------------//

template <class Prim>
struct Unary_Array_Func : public Function
{
    using Function::Function;
    using Op = Unary_Array_Op<Prim>;
    Value call(Value arg, Fail fl, Frame& f) const override
    {
        return Op::call(fl, At_Arg(*this, f), arg);
    }
    SC_Value sc_call_expr(Operation& argx, Shared<const Phrase> ph, SC_Frame& f)
    const override
    {
        return Op::sc_op(At_SC_Arg_Expr(*this, ph, f), argx, f);
    }
};

template <class Prim>
struct Binary_Array_Func : public Tuple_Function
{
    Binary_Array_Func(const char* nm) : Tuple_Function(2,nm) {}
    using Op = Binary_Array_Op<Prim>;
    Value tuple_call(Fail fl, Frame& args) const override
    {
        return Op::call(fl, At_Arg(*this, args), args[0], args[1]);
    }
    SC_Value sc_call_expr(Operation& argx, Shared<const Phrase> ph, SC_Frame& f)
    const override
    {
        return Op::sc_op(At_SC_Arg_Expr(*this, ph, f), argx, f);
    }
};

template <class Prim>
struct Monoid_Func final : public Function
{
    using Function::Function;
    using Op = Binary_Array_Op<Prim>;
    Value call(Value arg, Fail fl, Frame& f) const override
    {
        return Op::reduce(fl, At_Arg(*this, f), Prim::zero(), arg);
    }
    SC_Value sc_call_expr(Operation& argx, Shared<const Phrase> ph, SC_Frame& f)
    const override
    {
        return Op::sc_reduce(At_SC_Arg_Expr(*this, ph, f),
            Prim::zero(), argx, f);
    }
};

//-------------------//
// Builtin Functions //
//-------------------//

struct Is_Bool_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {is_bool(arg)};
    }
};
struct Is_Symbol_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {is_symbol(arg)};
    }
};
struct Is_Num_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {is_num(arg)};
    }
};
struct Is_String_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {is_string(arg)};
    }
};
struct Is_List_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {is_list(arg)};
    }
};
struct Is_Record_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {arg.maybe<Record>() != nullptr};
    }
};
struct Is_Primitive_Func_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {arg.maybe<Function>() != nullptr};
    }
};
struct Is_Func_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame& fr) const override
    {
        return {maybe_function(arg, At_Arg(*this, fr)) != nullptr};
    }
};

struct Bit_Prim : public Unary_Bool_To_Num_Prim
{
    static const char* name() { return "bit"; }
    static Value call(bool b, const Context&) { return {double(b)}; }
    static SC_Value sc_call(SC_Frame& f, SC_Value arg)
    {
        auto result = f.sc_.newvalue(SC_Type::Num(arg.type.count()));
        f.sc_.out() << "  " << result.type << " " << result << " = "
            << result.type << "(" << arg << ");\n";
        return result;
    }
};
using Bit_Function = Unary_Array_Func<Bit_Prim>;

#define UNARY_NUMERIC_FUNCTION(Func_Name,curv_name,c_name,glsl_name) \
struct Func_Name##Prim : public Unary_Num_SCVec_Prim \
{ \
    static const char* name() { return #curv_name; } \
    static Value call(double x, const Context&) { return {c_name(x)}; } \
    static SC_Value sc_call(SC_Frame& f, SC_Value arg) \
        { return sc_unary_call(f, arg.type, #glsl_name, arg); } \
}; \
using Func_Name = Unary_Array_Func<Func_Name##Prim>; \

UNARY_NUMERIC_FUNCTION(Sqrt_Function, sqrt, sqrt, sqrt)
UNARY_NUMERIC_FUNCTION(Log_Function, log, log, log)
UNARY_NUMERIC_FUNCTION(Abs_Function, abs, abs, abs)
UNARY_NUMERIC_FUNCTION(Floor_Function, floor, floor, floor)
UNARY_NUMERIC_FUNCTION(Ceil_Function, ceil, ceil, ceil)
UNARY_NUMERIC_FUNCTION(Trunc_Function, trunc, trunc, trunc)
UNARY_NUMERIC_FUNCTION(Round_Function, round, rint, roundEven)

inline double frac(double n) { return n - floor(n); }
UNARY_NUMERIC_FUNCTION(Frac_Function, frac, frac, fract)

UNARY_NUMERIC_FUNCTION(Sin_Function, sin, sin, sin)
UNARY_NUMERIC_FUNCTION(Cos_Function, cos, cos, cos)
UNARY_NUMERIC_FUNCTION(Tan_Function, tan, tan, tan)
UNARY_NUMERIC_FUNCTION(Acos_Function, acos, acos, acos)
UNARY_NUMERIC_FUNCTION(Asin_Function, asin, asin, asin)
UNARY_NUMERIC_FUNCTION(Atan_Function, atan, atan, atan)

UNARY_NUMERIC_FUNCTION(Sinh_Function, sinh, sinh, sinh)
UNARY_NUMERIC_FUNCTION(Cosh_Function, cosh, cosh, cosh)
UNARY_NUMERIC_FUNCTION(Tanh_Function, tanh, tanh, tanh)
UNARY_NUMERIC_FUNCTION(Acosh_Function, acosh, acosh, acosh)
UNARY_NUMERIC_FUNCTION(Asinh_Function, asinh, asinh, asinh)
UNARY_NUMERIC_FUNCTION(Atanh_Function, atanh, atanh, atanh)

struct Phase_Prim : public Unary_Vec2_To_Num_Prim
{
    static const char* name() { return "phase"; }
    static Value call(Vec2 v, const Context&) { return {atan2(v.y,v.x)}; }
    static SC_Value sc_call(SC_Frame& f, SC_Value arg) {
        auto result = f.sc_.newvalue(SC_Type::Num());
        f.sc_.out() << "  " << result.type << " " << result << " = "
            << "atan(" << arg << ".y," << arg << ".x);\n";
        return result;
    }
};
using Phase_Function = Unary_Array_Func<Phase_Prim>;

struct Max_Prim : public Binary_Num_SCVec_Prim
{
    static const char* name() { return "max"; }
    static Value zero() { return {-INFINITY}; }
    static Value call(double x, double y, const Context&)
        { return {std::max(x,y)}; }
    static SC_Value sc_call(SC_Frame& f, SC_Value x, SC_Value y)
        { return sc_bincall(f, x.type, "max", x, y); }
};
using Max_Function = Monoid_Func<Max_Prim>;

struct Min_Prim : public Binary_Num_SCVec_Prim
{
    static const char* name() { return "min"; }
    static Value zero() { return {INFINITY}; }
    static Value call(double x, double y, const Context&)
        { return {std::min(x,y)}; }
    static SC_Value sc_call(SC_Frame& f, SC_Value x, SC_Value y)
        { return sc_bincall(f, x.type, "min", x, y); }
};
using Min_Function = Monoid_Func<Min_Prim>;

using Sum_Function = Monoid_Func<Add_Prim>;

using Not_Function = Unary_Array_Func<Not_Prim>;

#define BOOL_OP(CppName,Name,Zero,LogOp,BitOp)\
struct CppName##_Prim : public Binary_Bool_Prim\
{\
    static const char* name() { return Name; } \
    static Value zero() { return {Zero}; }\
    static Value call(bool x, bool y, const Context&) { return {x LogOp y}; }\
    static SC_Value sc_call(SC_Frame& f, SC_Value x, SC_Value y)\
    {\
        auto result = f.sc_.newvalue(x.type);\
        f.sc_.out() << "  " << x.type << " " << result << " = ";\
        if (x.type.is_bool())\
            f.sc_.out() << x << #LogOp << y << ";\n";\
        else if (x.type.is_bool_or_vec()) {\
            /* In GLSL 4.6, I *think* you can use '&' and '|' instead. */ \
            /* TODO: SubCurv: more efficient and|or in bvec case */ \
            bool first = true;\
            f.sc_.out() << x.type << "(";\
            for (unsigned i = 0; i < x.type.count(); ++i) {\
                if (!first) f.sc_.out() << ",";\
                first = false;\
                f.sc_.out() << x << "[" << i << "]"\
                    << #LogOp << y << "[" << i << "]";\
            }\
            f.sc_.out() << ")";\
        }\
        else\
            f.sc_.out() << x << #BitOp << y << ";\n";\
        f.sc_.out() << ";\n";\
        return result;\
    }\
};\
using CppName##_Function = Monoid_Func<CppName##_Prim>;\

BOOL_OP(And,"and",true,&&,&)
BOOL_OP(Or,"or",false,||,|)

struct Xor_Prim : public Binary_Bool_Prim
{
    static const char* name() { return "xor"; }
    static Value zero() { return {false}; }
    static Value call(bool x, bool y, const Context&) { return {x != y}; }
    static SC_Value sc_call(SC_Frame& f, SC_Value x, SC_Value y)
    {
        auto result = f.sc_.newvalue(x.type);
        f.sc_.out() << "  " << result.type << " " << result << " = ";
        if (x.type.is_bool())
            f.sc_.out() << x << "!=" << y;
        else if (x.type.is_bool_or_vec())
            f.sc_.out() << "notEqual(" << x << "," << y << ")";
        else // bool32 or vector of bool32
            f.sc_.out() << x << "^" << y;
        f.sc_.out() << ";\n";
        return result;
    }
};
using Xor_Function = Monoid_Func<Xor_Prim>;

struct Lshift_Prim : public Shift_Prim
{
    static const char* name() { return "lshift"; }
    static Value call(Shared<const List> a, double b, const Context &cx)
    {
        At_Index acx(0, cx);
        At_Index bcx(1, cx);
        unsigned n = (unsigned) num_to_int(b, 0, a->size()-1, bcx);
        Shared<List> result = List::make(a->size());
        for (unsigned i = 0; i < n; ++i)
            result->at(i) = {false};
        for (unsigned i = n; i < a->size(); ++i)
            result->at(i) = {a->at(i-n).to_bool(acx)};
        return {result};
    }
    static SC_Value sc_call(SC_Frame& f, SC_Value x, SC_Value y)
    {
        auto result = f.sc_.newvalue(x.type);
        f.sc_.out() << "  " << x.type << " " << result << " = "
            << x << " << int(" << y << ");\n";
        return result;
    }
};
using Lshift_Function = Binary_Array_Func<Lshift_Prim>;

struct Rshift_Prim : public Shift_Prim
{
    static const char* name() { return "rshift"; }
    static Value call(Shared<const List> a, double b, const Context &cx)
    {
        At_Index acx(0, cx);
        At_Index bcx(1, cx);
        unsigned n = (unsigned) num_to_int(b, 0, a->size()-1, bcx);
        Shared<List> result = List::make(a->size());
        for (unsigned i = a->size()-n; i < a->size(); ++i)
            result->at(i) = {false};
        for (unsigned i = 0; i < a->size()-n; ++i)
            result->at(i) = {a->at(i+n).to_bool(acx)};
        return {result};
    }
    static SC_Value sc_call(SC_Frame& f, SC_Value x, SC_Value y)
    {
        auto result = f.sc_.newvalue(x.type);
        f.sc_.out() << "  " << x.type << " " << result << " = "
            << x << " >> int(" << y << ");\n";
        return result;
    }
};
using Rshift_Function = Binary_Array_Func<Rshift_Prim>;

struct Bool32_Sum_Prim : public Binary_Bool32_Prim
{
    static const char* name() { return "bool32_sum"; }
    static Value zero()
    {
        static Value z = {nat_to_bool32(0)};
        return z;
    }
    static Value call(unsigned a, unsigned b, const Context&)
    {
        return {nat_to_bool32(a + b)};
    }
    static SC_Value sc_call(SC_Frame& f, SC_Value x, SC_Value y)
    {
        auto result = f.sc_.newvalue(x.type);
        f.sc_.out() << "  " << x.type << " " << result << " = "
            << x << " + " << y << ";\n";
        return result;
    }
};
using Bool32_Sum_Function = Monoid_Func<Bool32_Sum_Prim>;

struct Bool32_Product_Prim : public Binary_Bool32_Prim
{
    static const char* name() { return "bool32_product"; }
    static Value zero()
    {
        static Value z = {nat_to_bool32(1)};
        return z;
    }
    static Value call(unsigned a, unsigned b, const Context&)
    {
        return {nat_to_bool32(a * b)};
    }
    static SC_Value sc_call(SC_Frame& f, SC_Value x, SC_Value y)
    {
        auto result = f.sc_.newvalue(x.type);
        f.sc_.out() << "  " << x.type << " " << result << " = "
            << x << " * " << y << ";\n";
        return result;
    }
};
using Bool32_Product_Function = Monoid_Func<Bool32_Product_Prim>;

struct Bool32_To_Nat_Function : public Function
{
    using Function::Function;
    struct Prim : public Unary_Bool32_To_Num_Prim
    {
        static const char* name() { return "bool32_to_nat"; }
        static Value call(unsigned n, const Context&)
        {
            return {double(n)};
        }
        // No SubCurv support because a Num (32 bit float)
        // cannot hold a Nat (32 bit natural).
        static SC_Type sc_result_type(SC_Type) { return {}; }
        static SC_Value sc_call(SC_Frame& f, SC_Value x)
        {
            throw Exception(At_SC_Frame(f),
                "bool32_to_nat is not supported by SubCurv");
        }
    };
    static Unary_Array_Op<Prim> array_op;
    Value call(Value arg, Fail fl, Frame& f) const override
    {
        return array_op.call(fl, At_Arg(*this, f), arg);
    }
};

struct Nat_To_Bool32_Function : public Function
{
    using Function::Function;
    struct Prim : public Unary_Num_To_Bool32_Prim
    {
        static const char* name() { return "nat_to_bool32"; }
        static Value call(double n, const Context& cx)
        {
            return {nat_to_bool32(num_to_nat(n, cx))};
        }
        static SC_Value sc_call(SC_Frame& f, SC_Value x)
        {
            throw Exception(At_SC_Frame(f),
                "nat_to_bool32 can't be called in this context: "
                "argument must be a constant");
        }
    };
    static Unary_Array_Op<Prim> array_op;
    Value call(Value arg, Fail fl, Frame& f) const override
    {
        return array_op.call(fl, At_Arg(*this, f), arg);
    }
    SC_Value sc_call_expr(Operation& argx, Shared<const Phrase> ph, SC_Frame& f)
    const override
    {
        At_SC_Arg_Expr cx(*this, ph, f);
        if (auto k = dynamic_cast<const Constant*>(&argx)) {
            unsigned n = num_to_nat(k->value_.to_num(cx), cx);
            auto type = SC_Type::Bool32();
            auto result = f.sc_.newvalue(type);
            f.sc_.out() << "  " << type << " " << result << " = "
                << n << "u;\n";
            return result;
        }
        else {
            throw Exception(cx, "argument must be a constant");
        }
    }
};

struct Bool32_To_Float_Prim : public Unary_Bool32_To_Num_Prim
{
    static const char* name() { return "bool32_to_float"; }
    static Value call(unsigned n, const Context&)
    {
        return {bitcast_nat_to_float(n)};
    }
    static SC_Value sc_call(SC_Frame& f, SC_Value x)
    {
        unsigned count = x.type.is_bool32() ? 1 : x.type.count();
        auto result = f.sc_.newvalue(SC_Type::Num(count));
        f.sc_.out() << "  " << result.type << " " << result
            << " = uintBitsToFloat(" << x << ");\n";
        return result;
    }
};
using Bool32_To_Float_Function = Unary_Array_Func<Bool32_To_Float_Prim>;

struct Float_To_Bool32_Prim : public Unary_Num_To_Bool32_Prim
{
    static const char* name() { return "bool32_to_float"; }
    static Value call(double n, const Context&)
    {
        return {nat_to_bool32(bitcast_float_to_nat(n))};
    }
    static SC_Value sc_call(SC_Frame& f, SC_Value x)
    {
        auto result = f.sc_.newvalue(SC_Type::Bool32(x.type.count()));
        f.sc_.out() << "  " << result.type << " " << result
            << " = floatBitsToUint(" << x << ");\n";
        return result;
    }
};
using Float_To_Bool32_Function = Unary_Array_Func<Float_To_Bool32_Prim>;

Value
select(Value a, Value b, Value c, Fail fl, const Context& cx)
{
    if (a.is_bool())
        return a.to_bool_unsafe() ? b : c;
    if (auto alist = a.maybe<List>()) {
        auto blist = b.maybe<List>();
        if (blist) blist->assert_size(alist->size(), At_Index(1, cx));
        auto clist = c.maybe<List>();
        if (clist) clist->assert_size(alist->size(), At_Index(2, cx));
        Shared<List> r = List::make(alist->size());
        for (unsigned i = 0; i < alist->size(); ++i) {
            TRY_DEF(v, select(alist->at(i),
                              blist ? blist->at(i) : b,
                              clist ? clist->at(i) : c,
                              fl, cx));
            r->at(i) = v;
        }
        return {r};
    }
    FAIL(fl, missing, At_Index(0, cx),
        stringify(a, " is not a Bool or a List"));
}

// `select[a,b,c]` is a vectorized version of `if` in which the condition is
// an array of booleans.
// * If `a` is boolean, then the result is `if (a) b else c`.
// * If `a` is an array of booleans, then the result is an array with the same
//   shape as `a`. The elements of the result are selected from the arrays `b`
//   and `c` based on whether the corresponding element of `a` is true or false.
//   For example, `select[[false,true], [1,2], [10,20]] == [10,2]`.
// * Broadcasting is supported between `a` and `b` and between `a` and `c`,
//   so for example, `select[[false,true], 1, 0] == [0,1]`.
// * Broadcasting is not supported between `b` and `c`, so for example,
//   `select[true, 1, [1,2,3]]` yields `1`, and not `[1,1,1]`.
// The SubCurv version of `select` restricts the arguments `b` and `c`
// to have the same type.
//
// `select` has a different name from `if` because it violates some of the
// laws of `if`: it always evaluates all 3 arguments, and it can't be involved
// in tail recursion optimization.
//
// Similar to: numpy.where, R `ifelse`
struct Select_Function : public Tuple_Function
{
    Select_Function(const char* nm) : Tuple_Function(3,nm) {}
    Value tuple_call(Fail fl, Frame& args) const override
    {
        return select(args[0], args[1], args[2], fl, At_Arg(*this, args));
    }
    SC_Value sc_tuple_call(SC_Frame& f) const override
    {
        auto cond = f[0];
        auto consequent = f[1];
        auto alternate = f[2];
        if (!cond.type.is_bool_or_vec()) {
            throw Exception(At_SC_Tuple_Arg(0,f), stringify(
                "argument is not bool or bool vector; it has type ",
                cond.type));
        }
        if (consequent.type != alternate.type) {
            throw Exception(At_SC_Tuple_Arg(1,f), stringify(
                "2nd and 3rd argument of 'select' have different types: ",
                consequent.type, " and ", alternate.type));
        }
        SC_Value result;
        if (cond.type.is_bool()) {
            result = f.sc_.newvalue(consequent.type);
            f.sc_.out() << "  " << result.type << " " << result << " = ";
            f.sc_.out() << cond << "?" << consequent << ":" << alternate;
        } else {
            // 'cond' is a boolean vector.
            if (consequent.type.count() == 1) {
                // Consequent & alternate are scalars. Convert them to vectors.
                auto T = SC_Type::List(consequent.type, cond.type.count());
                sc_try_extend(f, consequent, T);
                sc_try_extend(f, alternate, T);
            }
            else if (!consequent.type.is_vec()) {
                throw Exception(At_SC_Tuple_Arg(1,f), stringify(
                    "Must be a scalar or vector to match condition argument."
                    " Instead, type is ", consequent.type));
            }
            else if (cond.type.count() != consequent.type.count()) {
                throw Exception(At_SC_Tuple_Arg(1,f), stringify(
                    "Vector length ",consequent.type.count()," does not match"
                    " length of condition vector (", cond.type.count(),")"));
            }
            result = f.sc_.newvalue(consequent.type);
            f.sc_.out() << "  " << result.type << " " << result << " = ";
            // In GLSL 4.5, this is `mix(alt,cons,cond)` (all args are vectors).
            // Right now, we are locked to GLSL 3.3, so we can't use this.
            // TODO: SubCurv: more efficient `select` for vector case
            if (result.type.is_num_vec()) {
                // This version of 'mix' is linear interpolation: it works by
                // multiplication and addition of all 3 arguments. Which is
                // different from the boolean vector 'mix' in GLSL 4.5 (which
                // produces exact results even when linear interpolation would
                // fail due to floating point approximation). But I saw IQ use
                // linear interpolation of vectors to implement a 'select' in
                // WebGL, so maybe this is efficient code.
                f.sc_.out() << "mix(" << alternate << "," << consequent
                    << ",vec" << cond.type.count() << "(" << cond << "))";
            } else {
                f.sc_.out() << result.type << "(";
                bool atfirst = true;
                for (unsigned i = 0; i < result.type.count(); ++i) {
                    if (!atfirst) f.sc_.out() << ",";
                    atfirst = false;
                    f.sc_.out() << cond << "[" << i << "] ? "
                                << consequent << "[" << i << "] : "
                                << alternate << "[" << i << "]";
                }
                f.sc_.out() << ")";
            }
        }
        f.sc_.out() << ";\n";
        return result;
    }
};

struct Equal_Prim : public Binary_Scalar_To_Bool_Prim
{
    static const char* name() { return "equal"; }
    static Value call(Value a, Value b, const At_Syntax &cx)
    {
        return eqval<Binary_Op_Expr<Binary_Array_Op<Equal_Prim>>>(
            a.equal(b, cx), a, b, cx);
    }
    static SC_Value sc_call(SC_Frame& f, SC_Value x, SC_Value y)
    {
        auto result = f.sc_.newvalue(SC_Type::Bool(x.type.count()));
        f.sc_.out() << "  " << result.type << " " << result << " = ";
        if (x.type.is_vec()) {
            f.sc_.out() << "equal(" << x << "," << y << ")";
        } else {
            f.sc_.out() << x << "==" << y;
        }
        f.sc_.out() << ";\n";
        return result;
    }
};
using Equal_Function = Binary_Array_Func<Equal_Prim>;

struct Unequal_Prim : public Binary_Scalar_To_Bool_Prim
{
    static const char* name() { return "unequal"; }
    static Value call(Value a, Value b, const At_Syntax &cx)
    {
        return eqval<Binary_Op_Expr<Binary_Array_Op<Unequal_Prim>>>(
            !a.equal(b, cx), a, b, cx);
    }
    static SC_Value sc_call(SC_Frame& f, SC_Value x, SC_Value y)
    {
        auto result = f.sc_.newvalue(SC_Type::Bool(x.type.count()));
        f.sc_.out() << "  " << result.type << " " << result << " = ";
        if (x.type.is_vec()) {
            f.sc_.out() << "notEqual(" << x << "," << y << ")";
        } else {
            f.sc_.out() << x << "!=" << y;
        }
        f.sc_.out() << ";\n";
        return result;
    }
};
using Unequal_Function = Binary_Array_Func<Unequal_Prim>;

SC_Value Equal_Expr::sc_eval(SC_Frame& f) const
{
    auto a = sc_eval_op(f, *arg1_);
    auto b = sc_eval_op(f, *arg2_);
    if (a.type != b.type || a.type.plex_array_rank() > 0) {
        throw Exception(At_SC_Phrase(syntax_, f),
            stringify("domain error: ",a.type," == ",b.type));
    }
    SC_Value result = f.sc_.newvalue(SC_Type::Bool());
    f.sc_.out() <<"  bool "<<result<<" =("<<a<<" == "<<b<<");\n";
    return result;
}
SC_Value Not_Equal_Expr::sc_eval(SC_Frame& f) const
{
    auto a = sc_eval_op(f, *arg1_);
    auto b = sc_eval_op(f, *arg2_);
    if (a.type != b.type || a.type.plex_array_rank() > 0) {
        throw Exception(At_SC_Phrase(syntax_, f),
            stringify("domain error: ",a.type," != ",b.type));
    }
    SC_Value result = f.sc_.newvalue(SC_Type::Bool());
    f.sc_.out() <<"  bool "<<result<<" =("<<a<<" != "<<b<<");\n";
    return result;
}

// Generalized dot product that includes vector dot product and matrix product.
// Same as Mathematica Dot[A,B]. Like APL A+.×B, Python numpy.dot(A,B)
//  dot(a,b) =
//    if (count a > 0 && is_list(a[0]))
//      [for (row in a) dot(row,b)]  // matrix*...
//    else
//      sum(a*b)                     // vector*...
struct Dot_Function : public Tuple_Function
{
    Dot_Function(const char* nm) : Tuple_Function(2,nm) {}
    Value dot(Value a, Value b, Fail, const At_Arg& cx) const;
    Value tuple_call(Fail fl, Frame& args) const override
    {
        return dot(args[0], args[1], fl, At_Arg(*this, args));
    }
    SC_Value sc_tuple_call(SC_Frame& f) const override
    {
        auto a = f[0];
        auto b = f[1];
        if (a.type.is_num_vec() && a.type == b.type)
            return sc_bincall(f, SC_Type::Num(), "dot", a, b);
        if (a.type.is_num_vec() && b.type.is_mat()
            && a.type.count() == b.type.count())
        {
            return sc_binop(f, a.type, b, "*", a);
        }
        if (a.type.is_mat() && b.type.is_num_vec()
            && a.type.count() == b.type.count())
        {
            return sc_binop(f, b.type, b, "*", a);
        }
        if (a.type.is_mat() && b.type.is_mat()
            && a.type.count() == b.type.count())
        {
            return sc_binop(f, a.type, b, "*", a);
        }
        throw Exception(At_SC_Frame(f), "dot: invalid arguments");
    }
};
Value Dot_Function::dot(Value a, Value b, Fail fl, const At_Arg& cx) const
{
    auto av = a.maybe<List>();
    auto bv = b.maybe<List>();
    if (av && bv) {
        if (av->size() > 0 && av->at(0).maybe<List>()) {
            Shared<List> result = List::make(av->size());
            for (size_t i = 0; i < av->size(); ++i) {
                TRY_DEF(v, dot(av->at(i), b, fl, cx));
                result->at(i) = v;
            }
            return {result};
        } else {
            if (av->size() != bv->size())
                throw Exception(cx, stringify("list of size ",av->size(),
                    " can't be multiplied by list of size ",bv->size()));
            Value result = {0.0};
            for (size_t i = 0; i < av->size(); ++i) {
                TRY_DEF(prod, Multiply_Op::call(fl, cx, av->at(i), bv->at(i)));
                TRY_DEF(sum, Add_Op::call(fl, cx, result, prod));
                result = sum;
            }
            return result;
        }
    }
    // Handle the case where a or b is a reactive list,
    // and return a reactive result.
    //   This is copied and modified from Dot_Function::sc_tuple_call.
    //   The code ought to be identical in both cases.
    //   The reactive result should contain SubCurv IR code,
    //   which is simultaneously code that can be evaluated by the interpreter.
    auto aty = sc_type_of(a);
    auto bty = sc_type_of(b);
    SC_Type rty;
    if (aty.is_num_vec() && aty == bty)
        rty = SC_Type::Num();
    else if (aty.is_num_vec() && bty.is_mat() && aty.count() == bty.count())
        rty = bty;
    else if (aty.is_mat() && bty.is_num_vec() && aty.count() == bty.count())
        rty = aty;
    else if (aty.is_mat() && bty.is_mat() && aty.count() == bty.count())
        rty = aty;
    else
        throw Exception(cx, stringify("dot[",a,",",b,"]: invalid arguments"));
    Shared<List_Expr> args = List_Expr::make(
        {to_expr(a, cx.syntax()), to_expr(b, cx.syntax())},
        share(cx.syntax()));
    args->init();
    return {make<Reactive_Expression>(
        rty,
        make<Call_Expr>(
            cx.call_frame_.call_phrase_,
            make<Constant>(
                func_part(cx.call_frame_.call_phrase_),
                Value{share(*this)}),
            args),
        cx)};
}

struct Mag_Function : public Tuple_Function
{
    Mag_Function(const char* nm) : Tuple_Function(1,nm) {}
    Value tuple_call(Fail fl, Frame& args) const override
    {
        // Use hypot() or BLAS DNRM2 or Eigen stableNorm/blueNorm?
        // Avoids overflow/underflow due to squaring of large/small values.
        // Slower.  https://forum.kde.org/viewtopic.php?f=74&t=62402

        // Fast path: assume we have a list of numbers, compute a result.
        if (auto list = args[0].maybe<List>()) {
            double sum = 0.0;
            for (auto val : *list) {
                double x = val.to_num_or_nan();
                sum += x * x;
            }
            if (sum == sum)
                return {sqrt(sum)};
        }

        // Slow path, return a reactive value or abort.
        Shared<Operation> arg_op = nullptr;
        if (auto rx = args[0].maybe<Reactive_Value>()) {
            if (rx->sctype_.is_num_vec())
                arg_op = rx->expr();
        } else {
            TRY_DEF(list, args[0].to<List>(fl, At_Arg(*this, args)));
            Shared<List_Expr> rlist =
                List_Expr::make(list->size(),arg_part(args.call_phrase_));
            arg_op = rlist;
            for (unsigned i = 0; i < list->size(); ++i) {
                Value val = list->at(i);
                if (val.is_num()) {
                    rlist->at(i) =
                        make<Constant>(arg_part(args.call_phrase_), val);
                    continue;
                }
                auto r = val.maybe<Reactive_Value>();
                if (r && r->sctype_.is_num()) {
                    rlist->at(i) = r->expr();
                    continue;
                }
                arg_op = nullptr;
                break;
            }
            if (arg_op) rlist->init();
        }
        if (arg_op) {
            return {make<Reactive_Expression>(
                SC_Type::Num(),
                make<Call_Expr>(
                    args.call_phrase_,
                    make<Constant>(
                        func_part(args.call_phrase_),
                        Value{share(*this)}),
                    arg_op),
                At_Arg(*this, args))};
        }
        FAIL(fl, missing, At_Arg(*this, args),
            stringify(args[0],": domain error"));
    }
    SC_Value sc_tuple_call(SC_Frame& f) const override
    {
        auto arg = f[0];
        if (!arg.type.is_num_vec())
            throw Exception(At_SC_Tuple_Arg(0, f), "mag: argument is not a vector");
        auto result = f.sc_.newvalue(SC_Type::Num());
        f.sc_.out() << "  float "<<result<<" = length("<<arg<<");\n";
        return result;
    }
};

struct Count_Function : public Tuple_Function
{
    Count_Function(const char* nm) : Tuple_Function(1,nm) {}
    Value tuple_call(Fail fl, Frame& args) const override
    {
        if (auto list = args[0].maybe<const List>())
            return {double(list->size())};
        if (auto string = args[0].maybe<const String>())
            return {double(string->size())};
        if (auto re = args[0].maybe<const Reactive_Value>()) {
            if (re->sctype_.is_list())
                return {double(re->sctype_.count())};
        }
        FAIL(fl, missing, At_Arg(*this, args), "not a list or string");
    }
    SC_Value sc_tuple_call(SC_Frame& f) const override
    {
        auto arg = f[0];
        if (!arg.type.is_list())
            throw Exception(At_SC_Tuple_Arg(0, f), "count: argument is not a list");
        auto result = f.sc_.newvalue(SC_Type::Num());
        f.sc_.out() << "  float "<<result<<" = "<<arg.type.count()<<";\n";
        return result;
    }
};
struct Fields_Function : public Tuple_Function
{
    Fields_Function(const char* nm) : Tuple_Function(1,nm) {}
    static Value fields(Value arg, Fail fl, const Context& cx)
    {
        if (auto record = arg.maybe<const Record>())
            return {record->fields()};
      #if 0
        else if (auto list = arg.maybe<List>()) {
            Shared<List> result = List::make(list->size());
            for (unsigned i = 0; i < list->size(); ++i)
                result->at(i) = fields(list->at(i), cx);
            return {result};
        }
      #endif
        else
            FAIL(fl, missing, cx, stringify(arg, " is not a record"));
    }
    Value tuple_call(Fail fl, Frame& args) const override
    {
        return fields(args[0], fl, At_Arg(*this, args));
    }
};

struct Symbol_Function : public Function
{
    using Function::Function;
    virtual Value call(Value arg, Fail fl, Frame& fr) const override
    {
        TRY_DEF(string, arg.to<String>(fl, At_Arg(*this, fr)));
        auto symbol = make_symbol(string->data(), string->size());
        return symbol.to_value();
    }
};
struct Strcat_Function : public Tuple_Function
{
    Strcat_Function(const char* nm) : Tuple_Function(1,nm) {}
    Value tuple_call(Fail fl, Frame& args) const override
    {
        if (auto list = args[0].maybe<const List>()) {
            String_Builder sb;
            for (auto val : *list)
                val.print_string(sb);
            return {sb.get_string()};
        }
        FAIL(fl, missing, At_Arg(*this, args), "not a list");
    }
};
struct Repr_Function : public Tuple_Function
{
    Repr_Function(const char* nm) : Tuple_Function(1,nm) {}
    Value tuple_call(Fail, Frame& args) const override
    {
        String_Builder sb;
        sb << args[0];
        return {sb.get_string()};
    }
};
struct Decode_Function : public Tuple_Function
{
    Decode_Function(const char* nm) : Tuple_Function(1,nm) {}
    Value tuple_call(Fail fl, Frame& f) const override
    {
        String_Builder sb;
        At_Arg cx(*this, f);
        TRY_DEF(list, f[0].to<List>(fl, cx));
        for (size_t i = 0; i < list->size(); ++i)
            sb << (char)(*list)[i].to_int(1, 127, At_Index(i,cx));
        return {sb.get_string()};
    }
};
struct Encode_Function : public Tuple_Function
{
    Encode_Function(const char* nm) : Tuple_Function(1,nm) {}
    Value tuple_call(Fail fl, Frame& f) const override
    {
        List_Builder lb;
        At_Arg cx(*this, f);
        TRY_DEF(str, value_to_string(f[0], fl, cx));
        for (size_t i = 0; i < str->size(); ++i)
            lb.push_back({(double)(int)str->at(i)});
        return {lb.get_list()};
    }
};

struct Match_Function : public Function
{
    using Function::Function;
    virtual Value call(Value arg, Fail fl, Frame& f) const override
    {
        At_Arg ctx0(*this, f);
        TRY_DEF(list, arg.to<List>(fl, ctx0));
        std::vector<Shared<const Function>> cases;
        for (size_t i = 0; i < list->size(); ++i) {
            TRY_DEF(fn, value_to_function(list->at(i), fl, At_Index(i,ctx0)));
            cases.push_back(fn);
        }
        auto mf = make<Piecewise_Function>(cases);
        mf->name_ = name_;
        mf->argpos_ = 1;
        return {mf};
    }
};

struct Compose_Function : public Function
{
    using Function::Function;
    virtual Value call(Value arg, Fail fl, Frame& f) const override
    {
        At_Arg ctx0(*this, f);
        TRY_DEF(list, arg.to<List>(fl, ctx0));
        std::vector<Shared<const Function>> cases;
        for (size_t i = 0; i < list->size(); ++i) {
            TRY_DEF(fn, value_to_function(list->at(i), fl, At_Index(i,ctx0)));
            cases.push_back(fn);
        }
        auto mf = make<Composite_Function>(cases);
        mf->name_ = name_;
        mf->argpos_ = 1;
        return {mf};
    }
};

// The filename argument to "file", if it is a relative filename,
// is interpreted relative to the parent directory of the source file from
// which "file" is called.
//
// Because "file" has this hidden parameter (the name of the source file from
// which it is called), it is not a pure function. For this reason, it isn't
// a function value at all, it's a metafunction.
struct File_Expr : public Just_Expression
{
    Shared<Operation> arg_;
    File_Expr(Shared<const Call_Phrase> src, Shared<Operation> arg)
    :
        Just_Expression(std::move(src)),
        arg_(std::move(arg))
    {}
    virtual Value eval(Frame& f) const override
    {
        // Metafunction calls do not automatically get a new stack frame
        // allocated. But I want the call to `file pathname` to appear
        // in stack traces, so I need a Frame.
        auto& callphrase = dynamic_cast<const Call_Phrase&>(*syntax_);
        std::unique_ptr<Frame> f2 =
            Frame::make(0, f.system_, &f, &callphrase, nullptr);
        At_Metacall_With_Call_Frame cx("file", 0, *f2);

        // construct file pathname from argument
        Value arg = arg_->eval(f);
        auto argstr = value_to_string(arg, Fail::hard, cx);
        namespace fs = boost::filesystem;
        fs::path filepath;
        auto caller_filename = syntax_->location().source().name_;
        if (caller_filename->empty()) {
            filepath = fs::path(argstr->c_str());
        } else {
            filepath = fs::path(caller_filename->c_str()).parent_path()
                / fs::path(argstr->c_str());
        }

        return import(filepath, cx);
    }
};
struct File_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        return make<File_Expr>(share(ph), analyse_op(*ph.arg_, env));
    }
};

/// The meaning of a call to `print`, such as `print "foo"`.
struct Print_Action : public Operation
{
    Shared<Operation> arg_;
    Print_Action(
        Shared<const Phrase> syntax,
        Shared<Operation> arg)
    :
        Operation(std::move(syntax)),
        arg_(std::move(arg))
    {}
    virtual void exec(Frame& f, Executor&) const override
    {
        Value arg = arg_->eval(f);
        auto str = to_print_string(arg);
        f.system_.print(str->c_str());
    }
};
/// The meaning of the phrase `print` in isolation.
struct Print_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        return make<Print_Action>(share(ph), analyse_op(*ph.arg_, env));
    }
};

struct Warning_Action : public Operation
{
    Shared<Operation> arg_;
    Warning_Action(
        Shared<const Phrase> syntax,
        Shared<Operation> arg)
    :
        Operation(std::move(syntax)),
        arg_(std::move(arg))
    {}
    virtual void exec(Frame& f, Executor&) const override
    {
        Value arg = arg_->eval(f);
        auto msg = to_print_string(arg);
        Exception exc{At_Phrase(*syntax_, f), msg};
        f.system_.warning(exc);
    }
};
/// The meaning of the phrase `warning` in isolation.
struct Warning_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        return make<Warning_Action>(share(ph), analyse_op(*ph.arg_, env));
    }
};

struct Error_Function : public Function
{
    using Function::Function;
    virtual Value call(Value arg, Fail fl, Frame& fm) const override
    {
        FAIL(fl, missing, At_Frame(fm), to_print_string(arg));
    }
};
/// The meaning of a call to `error`, such as `error("foo")`.
struct Error_Operation : public Operation
{
    Shared<Operation> arg_;
    Error_Operation(
        Shared<const Phrase> syntax,
        Shared<Operation> arg)
    :
        Operation(std::move(syntax)),
        arg_(std::move(arg))
    {}
    [[noreturn]] void run(Frame& f) const
    {
        Value val = arg_->eval(f);
        auto msg = to_print_string(val);
        throw Exception{At_Phrase(*syntax_, f), msg};
    }
    virtual void exec(Frame& f, Executor&) const override
    {
        run(f);
    }
    virtual Value eval(Frame& f) const override
    {
        run(f);
    }
};
/// The meaning of the phrase `error` in isolation.
struct Error_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        return make<Error_Operation>(share(ph), analyse_op(*ph.arg_, env));
    }
    virtual Shared<Operation> to_operation(System&, Frame*) override
    {
        return make<Constant>(syntax_, Value{make<Error_Function>("error")});
    }
};

// exec(expr) -- a debug action that evaluates expr, then discards the result.
// It is used to call functions or source files for their side effects.
struct Exec_Action : public Operation
{
    Shared<Operation> arg_;
    Exec_Action(
        Shared<const Phrase> syntax,
        Shared<Operation> arg)
    :
        Operation(std::move(syntax)),
        arg_(std::move(arg))
    {}
    virtual void exec(Frame& f, Executor&) const override
    {
        arg_->eval(f);
    }
};
struct Exec_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        return make<Exec_Action>(share(ph), analyse_op(*ph.arg_, env));
    }
};

struct Assert_Action : public Operation
{
    Shared<Operation> arg_;
    Assert_Action(
        Shared<const Phrase> syntax,
        Shared<Operation> arg)
    :
        Operation(std::move(syntax)),
        arg_(std::move(arg))
    {}
    virtual void exec(Frame& f, Executor&) const override
    {
        At_Metacall cx{"assert", 0, *arg_->syntax_, f};
        bool b = arg_->eval(f).to_bool(cx);
        if (!b)
            throw Exception(At_Phrase(*syntax_, f), "assertion failed");
    }
};
struct Assert_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        auto arg = analyse_op(*ph.arg_, env);
        return make<Assert_Action>(share(ph), arg);
    }
};

struct Assert_Error_Action : public Operation
{
    Shared<Operation> expected_message_;
    Shared<const String> actual_message_;
    Shared<Operation> expr_;

    Assert_Error_Action(
        Shared<const Phrase> syntax,
        Shared<Operation> expected_message,
        Shared<const String> actual_message,
        Shared<Operation> expr)
    :
        Operation(std::move(syntax)),
        expected_message_(std::move(expected_message)),
        actual_message_(std::move(actual_message)),
        expr_(std::move(expr))
    {}

    virtual void exec(Frame& f, Executor&) const override
    {
        Value expected_msg_val = expected_message_->eval(f);
        auto expected_msg_str =
            value_to_string(expected_msg_val, Fail::hard,
                At_Phrase(*expected_message_->syntax_, f));

        if (actual_message_ != nullptr) {
            if (*actual_message_ != *expected_msg_str)
                throw Exception(At_Phrase(*syntax_, f),
                    stringify("assertion failed: expected error \"",
                        expected_msg_str,
                        "\", actual error \"",
                        actual_message_,
                        "\""));
            return;
        }

        Value result;
        try {
            result = expr_->eval(f);
        } catch (Exception& e) {
            if (*e.shared_what() != *expected_msg_str) {
                throw Exception(At_Phrase(*syntax_, f),
                    stringify("assertion failed: expected error \"",
                        expected_msg_str,
                        "\", actual error \"",
                        e.shared_what(),
                        "\""));
            }
            return;
        }
        throw Exception(At_Phrase(*syntax_, f),
            stringify("assertion failed: expected error \"",
                expected_msg_str,
                "\", got value ", result));
    }
};
struct Assert_Error_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        Shared<const Comma_Phrase> commas = nullptr;
        if (auto parens = cast<Paren_Phrase>(ph.arg_)) {
            commas = cast<Comma_Phrase>(parens->body_);
        }
        else if (auto brackets = cast<Bracket_Phrase>(ph.arg_)) {
            commas = cast<Comma_Phrase>(brackets->body_);
        }
        if (commas && commas->args_.size() == 2) {
            auto msg = analyse_op(*commas->args_[0].expr_, env);
            Shared<Operation> expr = nullptr;
            Shared<const String> actual_msg = nullptr;
            try {
                expr = analyse_op(*commas->args_[1].expr_, env);
            } catch (Exception& e) {
                actual_msg = e.shared_what();
            }
            return make<Assert_Error_Action>(share(ph), msg, actual_msg, expr);
        } else {
            throw Exception(At_Phrase(ph, env),
                "assert_error: expecting 2 arguments");
        }
    }
};

struct Defined_Expression : public Just_Expression
{
    Shared<const Operation> expr_;
    Symbol_Expr selector_;

    Defined_Expression(
        Shared<const Phrase> syntax,
        Shared<const Operation> expr,
        Symbol_Expr selector)
    :
        Just_Expression(std::move(syntax)),
        expr_(std::move(expr)),
        selector_(std::move(selector))
    {
    }

    static Value defined_at(Value val, Symbol_Ref id)
    {
        if (auto rec = val.maybe<Record>())
            return {rec->hasfield(id)};
      #if 0
        else if (auto list = val.maybe<List>()) {
            Shared<List> result = List::make(list->size());
            for (unsigned i = 0; i < list->size(); ++i)
                result->at(i) = defined_at(list->at(i), id);
            return {result};
        }
      #endif
        else
            return {false};
    }
    virtual Value eval(Frame& f) const override
    {
        auto val = expr_->eval(f);
        auto id = selector_.eval(f);
        return defined_at(val, id);
    }
};
struct Defined_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        auto arg = analyse_op(*ph.arg_, env);
        auto dot = cast<Dot_Expr>(arg);
        if (dot != nullptr)
            return make<Defined_Expression>(
                share(ph), dot->base_, dot->selector_);
        throw Exception(At_Phrase(*ph.arg_, env),
            "defined: argument must be `expression.identifier`");
    }
};

struct Builtin_Time : public Builtin
{
    virtual Shared<Meaning> to_meaning(const Identifier& id) const
    {
        return make<Constant>(share(id), Value{make<Uniform_Variable>(
            make_symbol("time"),
            std::string("u_time"),
            SC_Type::Num(),
            share(id))});
    }
};

const Namespace&
builtin_namespace()
{
    #define FUNCTION(nm,f) \
        {make_symbol(nm), make<Builtin_Value>(Value{make<f>(nm)})}

    static const Namespace names = {
    {make_symbol("pi"), make<Builtin_Value>(pi)},
    {make_symbol("tau"), make<Builtin_Value>(two_pi)},
    {make_symbol("inf"), make<Builtin_Value>(INFINITY)},
    {make_symbol("false"), make<Builtin_Value>(Value(false))},
    {make_symbol("true"), make<Builtin_Value>(Value(true))},
    {make_symbol("time"), make<Builtin_Time>()},

    FUNCTION("is_bool", Is_Bool_Function),
    FUNCTION("is_symbol", Is_Symbol_Function),
    FUNCTION("is_num", Is_Num_Function),
    FUNCTION("is_string", Is_String_Function),
    FUNCTION("is_list", Is_List_Function),
    FUNCTION("is_record", Is_Record_Function),
    FUNCTION("is_primitive_func", Is_Primitive_Func_Function),
    FUNCTION("is_func", Is_Func_Function),
    FUNCTION("bit", Bit_Function),
    FUNCTION("sqrt", Sqrt_Function),
    FUNCTION("log", Log_Function),
    FUNCTION("abs", Abs_Function),
    FUNCTION("floor", Floor_Function),
    FUNCTION("ceil", Ceil_Function),
    FUNCTION("trunc", Trunc_Function),
    FUNCTION("round", Round_Function),
    FUNCTION("frac", Frac_Function),
    FUNCTION("sin", Sin_Function),
    FUNCTION("cos", Cos_Function),
    FUNCTION("tan", Tan_Function),
    FUNCTION("asin", Asin_Function),
    FUNCTION("acos", Acos_Function),
    FUNCTION("atan", Atan_Function),
    FUNCTION("phase", Phase_Function),
    FUNCTION("sinh", Sinh_Function),
    FUNCTION("cosh", Cosh_Function),
    FUNCTION("tanh", Tanh_Function),
    FUNCTION("asinh", Asinh_Function),
    FUNCTION("acosh", Acosh_Function),
    FUNCTION("atanh", Atanh_Function),
    FUNCTION("max", Max_Function),
    FUNCTION("min", Min_Function),
    FUNCTION("sum", Sum_Function),
    FUNCTION("not", Not_Function),
    FUNCTION("and", And_Function),
    FUNCTION("or", Or_Function),
    FUNCTION("xor", Xor_Function),
    FUNCTION("lshift", Lshift_Function),
    FUNCTION("rshift", Rshift_Function),
    FUNCTION("bool32_sum", Bool32_Sum_Function),
    FUNCTION("bool32_product", Bool32_Product_Function),
    FUNCTION("bool32_to_nat", Bool32_To_Nat_Function),
    FUNCTION("nat_to_bool32", Nat_To_Bool32_Function),
    FUNCTION("bool32_to_float", Bool32_To_Float_Function),
    FUNCTION("float_to_bool32", Float_To_Bool32_Function),
    FUNCTION("select", Select_Function),
    FUNCTION("equal", Equal_Function),
    FUNCTION("unequal", Unequal_Function),
    FUNCTION("dot", Dot_Function),
    FUNCTION("mag", Mag_Function),
    FUNCTION("count", Count_Function),
    FUNCTION("fields", Fields_Function),
    FUNCTION("symbol", Symbol_Function),
    FUNCTION("strcat", Strcat_Function),
    FUNCTION("repr", Repr_Function),
    FUNCTION("decode", Decode_Function),
    FUNCTION("encode", Encode_Function),
    FUNCTION("match", Match_Function),
    FUNCTION("compose", Compose_Function),

    {make_symbol("file"), make<Builtin_Meaning<File_Metafunction>>()},
    {make_symbol("print"), make<Builtin_Meaning<Print_Metafunction>>()},
    {make_symbol("warning"), make<Builtin_Meaning<Warning_Metafunction>>()},
    {make_symbol("error"), make<Builtin_Meaning<Error_Metafunction>>()},
    {make_symbol("assert"), make<Builtin_Meaning<Assert_Metafunction>>()},
    {make_symbol("assert_error"), make<Builtin_Meaning<Assert_Error_Metafunction>>()},
    {make_symbol("exec"), make<Builtin_Meaning<Exec_Metafunction>>()},
    {make_symbol("defined"), make<Builtin_Meaning<Defined_Metafunction>>()},
    };
    return names;
}

} // namespace curv
