// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/builtin.h>

#include <libcurv/analyser.h>
#include <libcurv/array_op.h>
#include <libcurv/die.h>
#include <libcurv/dir_record.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/sc_context.h>
#include <libcurv/import.h>
#include <libcurv/math.h>
#include <libcurv/pattern.h>
#include <libcurv/picker.h>
#include <libcurv/prim.h>
#include <libcurv/program.h>
#include <libcurv/source.h>
#include <libcurv/system.h>
#include <libcurv/typeconv.h>

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
    Value call(Value arg, Frame& f) override
    {
        return Op::call(At_Arg(*this, f), arg);
    }
    SC_Value sc_call_expr(Operation& argx, Shared<const Phrase> ph, SC_Frame& f)
    const override
    {
        return Op::sc_op(At_SC_Arg_Expr(*this, ph, f), argx, f);
    }
};

template <class Prim>
struct Binary_Array_Func : public Legacy_Function
{
    Binary_Array_Func(const char* nm) : Legacy_Function(2,nm) {}
    using Op = Binary_Array_Op<Prim>;
    Value call(Frame& args) override
    {
        return Op::call(At_Arg(*this, args), args[0], args[1]);
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
    Value call(Value arg, Frame& f) override
    {
        return Op::reduce(At_Arg(*this, f), Prim::zero(), arg);
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
    Value call(Value arg, Frame&) override
    {
        return {isbool(arg)};
    }
};
struct Is_Symbol_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Frame&) override
    {
        return {issymbol(arg)};
    }
};
struct Is_Num_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Frame&) override
    {
        return {isnum(arg)};
    }
};
struct Is_String_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Frame&) override
    {
        return {arg.dycast<String>() != nullptr};
    }
};
struct Is_List_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Frame&) override
    {
        return {is_list(arg)};
    }
};
struct Is_Record_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Frame&) override
    {
        return {arg.dycast<Record>() != nullptr};
    }
};
struct Is_Fun_Function : public Function
{
    using Function::Function;
    Value call(Value arg, Frame&) override
    {
        return {arg.dycast<Function>() != nullptr};
    }
};

struct Bit_Prim : public Unary_Bool_To_Num_Prim
{
    static const char* name() { return "bit"; }
    static Value call(bool b, const Context&) { return {double(b)}; }
    static SC_Value sc_call(SC_Frame& f, SC_Value arg)
    {
        auto result = f.sc_.newvalue(SC_Type::Num_Or_Vec(arg.type.count()));
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

struct Atan2_Function : public Legacy_Function
{
    Atan2_Function(const char* nm) : Legacy_Function(2,nm) {}

    struct Prim {
        static double call(double x, double y) { return atan2(x, y); }
        Shared<Operation> make_expr(
            Shared<Operation> x, Shared<Operation> y) const
        {
            throw Exception(cx,
                "Internal error: atan2 applied to a reactive value");
            //return make<Divide_Expr>(share(syntax), std::move(x), std::move(y));
        }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify("[",x,",",y,"]");
        }
        At_Arg cx;
        Prim(Function& func, Frame& args) : cx(func, args) {}
    };
    static Binary_Numeric_Array_Op<Prim> array_op;
    Value call(Frame& args) override
    {
        return array_op.op(Prim(*this, args), args[0], args[1]);
    }
    SC_Value sc_call_legacy(SC_Frame& f) const override
    {
        auto x = f[0];
        auto y = f[1];

        SC_Type rtype = SC_Type::Bool();
        if (x.type == y.type)
            rtype = x.type;
        else if (x.type == SC_Type::Num())
            rtype = y.type;
        else if (y.type == SC_Type::Num())
            rtype = x.type;
        if (rtype == SC_Type::Bool())
            throw Exception(At_SC_Phrase(f.call_phrase_, f),
                "domain error");

        SC_Value result = f.sc_.newvalue(rtype);
        f.sc_.out() <<"  "<<rtype<<" "<<result<<" = atan(";
        sc_put_as(f, x, At_SC_Arg(0, f), rtype);
        f.sc_.out() << ",";
        sc_put_as(f, y, At_SC_Arg(1, f), rtype);
        f.sc_.out() << ");\n";
        return result;
    }
};

SC_Value sc_minmax(const char* name, Operation& argx, SC_Frame& f)
{
    auto list = dynamic_cast<List_Expr*>(&argx);
    if (list) {
        std::list<SC_Value> args;
        SC_Type type = SC_Type::Num();
        for (auto op : *list) {
            auto val = sc_eval_op(f, *op);
            args.push_back(val);
            if (val.type == SC_Type::Num())
                ;
            else if (val.type.is_num_vec()) {
                if (type == SC_Type::Num())
                    type = val.type;
                else if (type != val.type)
                    throw Exception(At_SC_Phrase(op->syntax_, f), stringify(
                        name, ": vector arguments of different lengths"));
            } else {
                throw Exception(At_SC_Phrase(op->syntax_, f), stringify(
                    name,": argument has bad type"));
            }
        }
        auto result = f.sc_.newvalue(type);
        if (args.size() == 0) {
            // TODO: BUG: this only works for 'max'. min requires +inf.
            f.sc_.out() << "  " << type << " " << result << " = -0.0/0.0;\n";
        }
        else if (args.size() == 1)
            return args.front();
        else {
            f.sc_.out() << "  " << type << " " << result << " = ";
            int rparens = 0;
            while (args.size() > 2) {
                f.sc_.out() << name << "(" << args.front() << ",";
                args.pop_front();
                ++rparens;
            }
            f.sc_.out() << name << "(" << args.front() << "," << args.back() << ")";
            while (rparens > 0) {
                f.sc_.out() << ")";
                --rparens;
            }
            f.sc_.out() << ";\n";
        }
        return result;
    } else {
        auto arg = sc_eval_op(f, argx);
        auto result = f.sc_.newvalue(SC_Type::Num());
        f.sc_.out() << "  float "<<result<<" = ";
        if (arg.type == SC_Type::Vec(2))
            f.sc_.out() << name <<"("<<arg<<".x,"<<arg<<".y);\n";
        else if (arg.type == SC_Type::Vec(3))
            f.sc_.out() << name<<"("<<name<<"("<<arg<<".x,"<<arg<<".y),"
                <<arg<<".z);\n";
        else if (arg.type == SC_Type::Vec(4))
            f.sc_.out() << name<<"("<<name<<"("<<name<<"("<<arg<<".x,"<<arg<<".y),"
                <<arg<<".z),"<<arg<<".w);\n";
        else
            throw Exception(At_SC_Phrase(argx.syntax_, f), stringify(
                name,": argument is not a vector"));
        return result;
    }
}

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

#define BOOL_OP(CppName,Name,Zero,LogOp,BitOp)\
struct CppName##_Prim : public Binary_Bool_Or_Bool32_Prim\
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

struct Xor_Prim : public Binary_Bool_Or_Bool32_Prim
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
    Value call(Value arg, Frame& f) override
    {
        return array_op.call(At_Arg(*this, f), arg);
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
    Value call(Value arg, Frame& f) override
    {
        return array_op.call(At_Arg(*this, f), arg);
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
        return {nat_to_float(n)};
    }
    static SC_Value sc_call(SC_Frame& f, SC_Value x)
    {
        unsigned count = x.type == SC_Type::Bool32() ? 1 : x.type.count();
        auto result = f.sc_.newvalue(SC_Type::Num_Or_Vec(count));
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
        return {nat_to_bool32(float_to_nat(n))};
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
select(Value a, Value b, Value c, const Context& cx)
{
    if (a.is_bool())
        return a.to_bool_unsafe() ? b : c;
    if (auto alist = a.dycast<List>()) {
        auto blist = b.dycast<List>();
        if (blist) blist->assert_size(alist->size(), At_Index(1, cx));
        auto clist = c.dycast<List>();
        if (clist) clist->assert_size(alist->size(), At_Index(2, cx));
        Shared<List> r = List::make(alist->size());
        for (unsigned i = 0; i < alist->size(); ++i) {
            r->at(i) = select(alist->at(i),
                              blist ? blist->at(i) : b,
                              clist ? clist->at(i) : c,
                              cx);
        }
        return {r};
    }
    throw Exception(At_Index(0, cx), stringify(a, " is not a Bool or a List"));
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
struct Select_Function : public Legacy_Function
{
    Select_Function(const char* nm) : Legacy_Function(3,nm) {}
    Value call(Frame& args) override
    {
        return select(args[0], args[1], args[2], At_Arg(*this, args));
    }
    SC_Value sc_call_legacy(SC_Frame& f) const override
    {
        auto cond = f[0];
        auto consequent = f[1];
        auto alternate = f[2];
        if (!cond.type.is_bool_or_vec()) {
            throw Exception(At_SC_Arg(0,f), stringify(
                "argument is not bool or bool vector; it has type ",
                cond.type));
        }
        if (consequent.type != alternate.type) {
            throw Exception(At_SC_Arg(1,f), stringify(
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
            else if (!consequent.type.is_any_vec()) {
                throw Exception(At_SC_Arg(1,f), stringify(
                    "Must be a scalar or vector to match condition argument."
                    " Instead, type is ", consequent.type));
            }
            else if (cond.type.count() != consequent.type.count()) {
                throw Exception(At_SC_Arg(1,f), stringify(
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

struct Equal_Prim : public Binary_Scalar_Prim
{
    static const char* name() { return "equal"; }
    static Value call(Value a, Value b, const Context &cx)
    {
        return {a.equal(b, cx)};
    }
    static SC_Value sc_call(SC_Frame& f, SC_Value x, SC_Value y)
    {
        auto result = f.sc_.newvalue(SC_Type::Bool(x.type.count()));
        f.sc_.out() << "  " << result.type << " " << result << " = ";
        if (x.type.is_any_vec()) {
            f.sc_.out() << "equal(" << x << "," << y << ")";
        } else {
            f.sc_.out() << x << "==" << y;
        }
        f.sc_.out() << ";\n";
        return result;
    }
};
using Equal_Function = Binary_Array_Func<Equal_Prim>;
struct Unequal_Prim : public Binary_Scalar_Prim
{
    static const char* name() { return "unequal"; }
    static Value call(Value a, Value b, const Context &cx)
    {
        return {!a.equal(b, cx)};
    }
    static SC_Value sc_call(SC_Frame& f, SC_Value x, SC_Value y)
    {
        auto result = f.sc_.newvalue(SC_Type::Bool(x.type.count()));
        f.sc_.out() << "  " << result.type << " " << result << " = ";
        if (x.type.is_any_vec()) {
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
    if (a.type != b.type || a.type.rank_ > 0) {
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
    if (a.type != b.type || a.type.rank_ > 0) {
        throw Exception(At_SC_Phrase(syntax_, f),
            stringify("domain error: ",a.type," != ",b.type));
    }
    SC_Value result = f.sc_.newvalue(SC_Type::Bool());
    f.sc_.out() <<"  bool "<<result<<" =("<<a<<" != "<<b<<");\n";
    return result;
}

// Generalized dot product that includes vector dot product and matrix product.
// Same as Mathematica Dot[A,B]. Like APL A+.×B, Python numpy.dot(A,B)
struct Dot_Function : public Legacy_Function
{
    Dot_Function(const char* nm) : Legacy_Function(2,nm) {}
    Value call(Frame& args) override
    {
        return dot(args[0], args[1], At_Arg(*this, args));
    }
    SC_Value sc_call_legacy(SC_Frame& f) const override
    {
        auto a = f[0];
        auto b = f[1];
        if (!a.type.is_num_vec())
            throw Exception(At_SC_Arg(0, f), "dot: argument is not a vector");
        if (a.type != b.type)
            throw Exception(At_SC_Arg(1, f), "dot: arguments have different types");
        auto result = f.sc_.newvalue(SC_Type::Num());
        f.sc_.out() << "  float "<<result<<" = dot("<<a<<","<<b<<");\n";
        return result;
    }
};

struct Mag_Function : public Legacy_Function
{
    Mag_Function(const char* nm) : Legacy_Function(1,nm) {}
    Value call(Frame& args) override
    {
        // TODO: use hypot() or BLAS DNRM2 or Eigen stableNorm/blueNorm?
        // Avoids overflow/underflow due to squaring of large/small values.
        // Slower.  https://forum.kde.org/viewtopic.php?f=74&t=62402
        auto list = args[0].to<List>(At_Arg(*this, args));
        // Fast path: assume we have a list of number, compute a result.
        double sum = 0.0;
        for (auto val : *list) {
            double x = val.to_num_or_nan();
            sum += x * x;
        }
        if (sum == sum)
            return {sqrt(sum)};
        // The computation failed. Second fastest path: assume a mix of numbers
        // and reactive numbers, try to return a reactive result.
        Shared<List_Expr> rlist =
            List_Expr::make(list->size(),arg_part(args.call_phrase_));
        for (unsigned i = 0; i < list->size(); ++i) {
            Value val = list->at(i);
            if (val.is_num()) {
                rlist->at(i) = make<Constant>(arg_part(args.call_phrase_), val);
                continue;
            }
            auto r = val.dycast<Reactive_Value>();
            if (r && r->sctype_ == SC_Type::Num()) {
                rlist->at(i) = r->expr();
                continue;
            }
            rlist = nullptr;
            break;
        }
        if (rlist) {
            rlist->init();
            return {make<Reactive_Expression>(
                SC_Type::Num(),
                make<Call_Expr>(
                    args.call_phrase_,
                    make<Constant>(
                        func_part(args.call_phrase_),
                        Value{share(*this)}),
                    rlist),
                At_Arg(*this, args))};
        }
        throw Exception(At_Arg(*this, args),
            stringify(args[0],": domain error"));
    }
    SC_Value sc_call_legacy(SC_Frame& f) const override
    {
        auto arg = f[0];
        if (!arg.type.is_num_vec())
            throw Exception(At_SC_Arg(0, f), "mag: argument is not a vector");
        auto result = f.sc_.newvalue(SC_Type::Num());
        f.sc_.out() << "  float "<<result<<" = length("<<arg<<");\n";
        return result;
    }
};

struct Count_Function : public Legacy_Function
{
    Count_Function(const char* nm) : Legacy_Function(1,nm) {}
    Value call(Frame& args) override
    {
        if (auto list = args[0].dycast<const List>())
            return {double(list->size())};
        if (auto string = args[0].dycast<const String>())
            return {double(string->size())};
        if (auto re = args[0].dycast<const Reactive_Value>()) {
            if (re->sctype_.is_list())
                return {double(re->sctype_.count())};
        }
        throw Exception(At_Arg(*this, args), "not a list or string");
    }
    SC_Value sc_call_legacy(SC_Frame& f) const override
    {
        auto arg = f[0];
        if (!arg.type.is_list())
            throw Exception(At_SC_Arg(0, f), "count: argument is not a list");
        auto result = f.sc_.newvalue(SC_Type::Num());
        f.sc_.out() << "  float "<<result<<" = "<<arg.type.count()<<";\n";
        return result;
    }
};
struct Fields_Function : public Legacy_Function
{
    Fields_Function(const char* nm) : Legacy_Function(1,nm) {}
    Value call(Frame& args) override
    {
        if (auto record = args[0].dycast<const Record>())
            return {record->fields()};
        throw Exception(At_Arg(*this, args), "not a record");
    }
};

struct Strcat_Function : public Legacy_Function
{
    Strcat_Function(const char* nm) : Legacy_Function(1,nm) {}
    Value call(Frame& args) override
    {
        if (auto list = args[0].dycast<const List>()) {
            String_Builder sb;
            for (auto val : *list) {
                if (auto str = val.dycast<const String_or_Symbol>())
                    sb << *str;
                else if (val.is_bool())
                    sb << (val.to_bool_unsafe() ? "true" : "false");
                else
                    sb << val;
            }
            return {sb.get_string()};
        }
        throw Exception(At_Arg(*this, args), "not a list");
    }
};
struct Repr_Function : public Legacy_Function
{
    Repr_Function(const char* nm) : Legacy_Function(1,nm) {}
    Value call(Frame& args) override
    {
        String_Builder sb;
        sb << args[0];
        return {sb.get_string()};
    }
};
struct Decode_Function : public Legacy_Function
{
    Decode_Function(const char* nm) : Legacy_Function(1,nm) {}
    Value call(Frame& f) override
    {
        String_Builder sb;
        At_Arg cx(*this, f);
        auto list = f[0].to<List>(cx);
        for (size_t i = 0; i < list->size(); ++i)
            sb << (char)(*list)[i].to_int(1, 127, At_Index(i,cx));
        return {sb.get_string()};
    }
};
struct Encode_Function : public Legacy_Function
{
    Encode_Function(const char* nm) : Legacy_Function(1,nm) {}
    Value call(Frame& f) override
    {
        List_Builder lb;
        At_Arg cx(*this, f);
        auto str = f[0].to<String>(cx);
        for (size_t i = 0; i < str->size(); ++i)
            lb.push_back({(double)(int)str->at(i)});
        return {lb.get_list()};
    }
};

struct Match_Function : public Legacy_Function
{
    Match_Function(const char* nm) : Legacy_Function(1,nm) {}
    Value call(Frame& f) override
    {
        At_Arg ctx0(*this, f);
        auto list = f[0].to<List>(ctx0);
        std::vector<Shared<Function>> cases;
        for (size_t i = 0; i < list->size(); ++i)
            cases.push_back(list->at(i).to<Function>(At_Index(i,ctx0)));
        auto mf = make<Piecewise_Function>(cases);
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
        auto argstr = arg.to<String>(cx);
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
        auto str = arg.dycast<String>();
        if (str == nullptr)
            str = stringify(arg);
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
        Shared<String> msg;
        if (auto str = arg.dycast<String>())
            msg = str;
        else
            msg = stringify(arg);
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
        Shared<const String> msg;
        if (auto s = val.dycast<String>())
            msg = s;
        else
            msg = stringify(val);
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
        auto expected_msg_str = expected_msg_val.to<const String>(
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
        auto parens = cast<Paren_Phrase>(ph.arg_);
        Shared<Comma_Phrase> commas = nullptr;
        if (parens) commas = cast<Comma_Phrase>(parens->body_);
        if (parens && commas && commas->args_.size() == 2) {
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

    virtual Value eval(Frame& f) const override
    {
        auto val = expr_->eval(f);
        auto s = val.dycast<Record>();
        if (s) {
            auto id = selector_.eval(f);
            return {s->hasfield(id)};
        } else {
            return {false};
        }
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

    FUNCTION("is_bool", Is_Bool_Function),
    FUNCTION("is_symbol", Is_Symbol_Function),
    FUNCTION("is_num", Is_Num_Function),
    FUNCTION("is_string", Is_String_Function),
    FUNCTION("is_list", Is_List_Function),
    FUNCTION("is_record", Is_Record_Function),
    FUNCTION("is_fun", Is_Fun_Function),
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
    FUNCTION("atan2", Atan2_Function),
    FUNCTION("sinh", Sinh_Function),
    FUNCTION("cosh", Cosh_Function),
    FUNCTION("tanh", Tanh_Function),
    FUNCTION("asinh", Asinh_Function),
    FUNCTION("acosh", Acosh_Function),
    FUNCTION("atanh", Atanh_Function),
    FUNCTION("max", Max_Function),
    FUNCTION("min", Min_Function),
    FUNCTION("sum", Sum_Function),
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
    FUNCTION("strcat", Strcat_Function),
    FUNCTION("repr", Repr_Function),
    FUNCTION("decode", Decode_Function),
    FUNCTION("encode", Encode_Function),
    FUNCTION("match", Match_Function),

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
