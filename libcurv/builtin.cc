// Copyright 2016-2019 Doug Moen
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

struct Is_Bool_Function : public Legacy_Function
{
    static const char* name() { return "is_bool"; }
    Is_Bool_Function() : Legacy_Function(1,name()) {}
    Value call(Frame& args) override
    {
        return {isbool(args[0])};
    }
};
struct Is_Symbol_Function : public Legacy_Function
{
    static const char* name() { return "is_symbol"; }
    Is_Symbol_Function() : Legacy_Function(1,name()) {}
    Value call(Frame& args) override
    {
        return {issymbol(args[0])};
    }
};
struct Is_Num_Function : public Legacy_Function
{
    static const char* name() { return "is_num"; }
    Is_Num_Function() : Legacy_Function(1,name()) {}
    Value call(Frame& args) override
    {
        return {isnum(args[0])};
    }
};
struct Is_String_Function : public Legacy_Function
{
    static const char* name() { return "is_string"; }
    Is_String_Function() : Legacy_Function(1,name()) {}
    Value call(Frame& args) override
    {
        return {args[0].dycast<String>() != nullptr};
    }
};
struct Is_List_Function : public Legacy_Function
{
    static const char* name() { return "is_list"; }
    Is_List_Function() : Legacy_Function(1,name()) {}
    Value call(Frame& args) override
    {
        return {islist(args[0])};
    }
};
struct Is_Record_Function : public Legacy_Function
{
    static const char* name() { return "is_record"; }
    Is_Record_Function() : Legacy_Function(1,name()) {}
    Value call(Frame& args) override
    {
        return {args[0].dycast<Record>() != nullptr};
    }
};
struct Is_Fun_Function : public Legacy_Function
{
    static const char* name() { return "is_fun"; }
    Is_Fun_Function() : Legacy_Function(1,name()) {}
    Value call(Frame& args) override
    {
        return {args[0].dycast<Function>() != nullptr};
    }
};

struct Bit_Function : public Legacy_Function
{
    static const char* name() { return "bit"; }
    Bit_Function() : Legacy_Function(1,name()) {}
    Value call(Frame& args) override
    {
        return {double(args[0].to_bool(At_Arg(*this, args)))};
    }
    SC_Value sc_call(SC_Frame& f) const override
    {
        auto arg = f[0];
        if (arg.type != SC_Type::Bool())
            throw Exception(At_SC_Arg(0, f),
                stringify(name(),": argument is not a bool"));
        auto result = f.sc_.newvalue(SC_Type::Num());
        f.sc_.out() << "  float "<<result<<" = float("<<arg<<");\n";
        return result;
    }
};

#define UNARY_NUMERIC_FUNCTION(Class_Name,curv_name,c_name,glsl_name) \
struct Class_Name : public Legacy_Function \
{ \
    static const char* name() { return #curv_name; } \
    Class_Name() : Legacy_Function(1,name()) {} \
    struct Scalar_Op { \
        static double call(double x) { return c_name(x); } \
        Shared<Operation> make_expr(Shared<Operation> x) const \
        { \
            return make<Call_Expr>( \
                cx.call_frame_.call_phrase_, \
                make<Constant>( \
                    func_part(cx.call_frame_.call_phrase_), \
                    Value{share(cx.func_)}), \
                x); \
        } \
        static Shared<const String> callstr(Value x) { \
            return stringify(x); \
        } \
        At_Arg cx; \
        Scalar_Op(Function& func, Frame& args) : cx(func,args) {} \
    }; \
    static Unary_Numeric_Array_Op<Scalar_Op> array_op; \
    Value call(Frame& args) override \
    { \
        return array_op.op(Scalar_Op(*this, args), args[0]); \
    } \
    SC_Value sc_call(SC_Frame& f) const override \
    { \
        return sc_call_unary_numeric(f, #glsl_name); \
    } \
}; \

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

double fhash(double f)
{
    union {
        double f;
        uint64_t u;
    } data;
    data.f = f;
    data.u =
        ((data.u & 0x000F'FFFF'FFFF'FFFF) // strip sign bit and exponent
        | 0x3FF0'0000'0000'0000) // set sign to 0 and exponent to 0
        ^ (data.u >> 12) // xor exponent and sign on top of mantissa
        ;
    // If f is normalized, then 1 <= data.f < 2.
    // If f is denormalized, then 0 <= data.f < 1. This includes f==0.
    return data.f - floor(data.f);
}
UNARY_NUMERIC_FUNCTION(Fhash_Function, fhash, fhash, fhash)

struct Atan2_Function : public Legacy_Function
{
    static const char* name() { return "atan2"; }
    Atan2_Function() : Legacy_Function(2,name()) {}

    struct Scalar_Op {
        static double call(double x, double y) { return atan2(x, y); }
        Shared<Operation> make_expr(
            Shared<Operation> x, Shared<Operation> y) const
        {
            throw Exception(cx,
                "Internal error: atan2 applied to a reactive value");
            //return make<Divide_Expr>(share(syntax), std::move(x), std::move(y));
        }
        static const char* name() { return "atan2"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify("[",x,",",y,"]");
        }
        At_Arg cx;
        Scalar_Op(Function& func, Frame& args) : cx(func, args) {}
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    Value call(Frame& args) override
    {
        return array_op.op(Scalar_Op(*this, args), args[0], args[1]);
    }
    SC_Value sc_call(SC_Frame& f) const override
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
            else if (val.type.is_vec()) {
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
        if (args.size() == 0)
            f.sc_.out() << "  " << type << " " << result << " = -0.0/0.0;\n";
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

struct Max_Function : public Legacy_Function
{
    static const char* name() { return "max"; }
    Max_Function() : Legacy_Function(1,name()) {}

    struct Scalar_Op {
        static double call(double x, double y) {
            // return NaN if either argument is NaN.
            if (x >= y) return x;
            if (x < y) return y;
            return 0.0/0.0;
        }
        Shared<Operation> make_expr(
            Shared<Operation> x, Shared<Operation> y) const
        {
            Shared<List_Expr> args =
                List_Expr::make({x, y}, arg_part(cx.call_frame_.call_phrase_));
            args->init();
            return make<Call_Expr>(
                cx.call_frame_.call_phrase_,
                make<Constant>(
                    func_part(cx.call_frame_.call_phrase_),
                    Value{share(cx.func_)}),
                args);
        }
        static const char* name() { return "max"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify("[",x,",",y,"]");
        }
        At_Arg cx;
        Scalar_Op(Function& func, Frame& args) : cx(func,args) {}
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    Value call(Frame& args) override
    {
        return array_op.reduce(Scalar_Op(*this, args), -INFINITY, args[0]);
    }
    SC_Value sc_call_expr(Operation& argx, Shared<const Phrase>, SC_Frame& f)
    const override
    {
        return sc_minmax(name(),argx,f);
    }
};

struct Min_Function : public Legacy_Function
{
    static const char* name() { return "min"; }
    Min_Function() : Legacy_Function(1,name()) {}

    struct Scalar_Op {
        static double call(double x, double y) {
            // return NaN if either argument is NaN
            if (x <= y) return x;
            if (x > y) return y;
            return 0.0/0.0;
        }
        Shared<Operation> make_expr(
            Shared<Operation> x, Shared<Operation> y) const
        {
            Shared<List_Expr> args =
                List_Expr::make({x, y}, arg_part(cx.call_frame_.call_phrase_));
            args->init();
            return make<Call_Expr>(
                cx.call_frame_.call_phrase_,
                make<Constant>(
                    func_part(cx.call_frame_.call_phrase_),
                    Value{share(cx.func_)}),
                args);
        }
        static const char* name() { return "min"; }
        static Shared<const String> callstr(Value x, Value y) {
            return stringify("[",x,",",y,"]");
        }
        At_Arg cx;
        Scalar_Op(Function& func, Frame& args) : cx(func, args) {}
    };
    static Binary_Numeric_Array_Op<Scalar_Op> array_op;
    Value call(Frame& args) override
    {
        return array_op.reduce(Scalar_Op(*this, args), INFINITY, args[0]);
    }
    SC_Value sc_call_expr(Operation& argx, Shared<const Phrase>, SC_Frame& f)
    const override
    {
        return sc_minmax("min",argx,f);
    }
};

struct And_Function : public Legacy_Function
{
    static const char* name() { return "and"; }
    And_Function() : Legacy_Function(1,name()) {}
    struct And_Op : public Binary_Boolean_Op
    {
        using Binary_Boolean_Op::Binary_Boolean_Op;
        static Value call(bool x, bool y) { return {x && y}; }
    };
    static Binary_Array_Op<And_Op> array_op;
    Value call(Frame& args) override
    {
        return array_op.reduce(And_Op(*this, args), Value{true}, args[0]);
    }
};
struct Or_Function : public Legacy_Function
{
    static const char* name() { return "or"; }
    Or_Function() : Legacy_Function(1,name()) {}
    struct Or_Op : public Binary_Boolean_Op
    {
        using Binary_Boolean_Op::Binary_Boolean_Op;
        static Value call(bool x, bool y) { return {x || y}; }
    };
    static Binary_Array_Op<Or_Op> array_op;
    Value call(Frame& args) override
    {
        return array_op.reduce(Or_Op(*this, args), Value{false}, args[0]);
    }
};
struct Xor_Function : public Legacy_Function
{
    static const char* name() { return "xor"; }
    Xor_Function() : Legacy_Function(1,name()) {}
    struct Xor_Op : public Binary_Boolean_Op
    {
        using Binary_Boolean_Op::Binary_Boolean_Op;
        static Value call(bool x, bool y) { return {x != y}; }
    };
    static Binary_Array_Op<Xor_Op> array_op;
    Value call(Frame& args) override
    {
        return array_op.reduce(Xor_Op(*this, args), Value{false}, args[0]);
    }
};
struct Lshift_Function : public Legacy_Function
{
    static const char* name() { return "lshift"; }
    Lshift_Function() : Legacy_Function(2,name()) {}
    struct Lshift_Op : public Shift_Op
    {
        using Shift_Op::Shift_Op;
        Value call(Shared<const List> a, double b) const
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
    };
    static Binary_Array_Op<Lshift_Op> array_op;
    Value call(Frame& args) override
    {
        return array_op.op(Lshift_Op(*this, args), args[0], args[1]);
    }
};
struct Rshift_Function : public Legacy_Function
{
    static const char* name() { return "rshift"; }
    Rshift_Function() : Legacy_Function(2,name()) {}
    struct Rshift_Op : public Shift_Op
    {
        using Shift_Op::Shift_Op;
        Value call(Shared<const List> a, double b) const
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
    };
    static Binary_Array_Op<Rshift_Op> array_op;
    Value call(Frame& args) override
    {
        return array_op.op(Rshift_Op(*this, args), args[0], args[1]);
    }
};
struct Bool32_Add_Function : public Legacy_Function
{
    static const char* name() { return "bool32_add"; }
    Bool32_Add_Function() : Legacy_Function(2,name()) {}
    struct Bool32_Add_Op : public Binary_Bool32_Op
    {
        using Binary_Bool32_Op::Binary_Bool32_Op;
        Value call(unsigned a, unsigned b) const
        {
            return {nat_to_bool32(a + b)};
        }
    };
    static Binary_Array_Op<Bool32_Add_Op> array_op;
    Value call(Frame& args) override
    {
        return array_op.op(Bool32_Add_Op(*this, args), args[0], args[1]);
    }
};
struct Bool32_To_Nat_Function : public Legacy_Function
{
    static const char* name() { return "bool32_to_nat"; }
    Bool32_To_Nat_Function() : Legacy_Function(1,name()) {}
    struct Scalar_Op : public Unary_Bool32_Op
    {
        using Unary_Bool32_Op::Unary_Bool32_Op;
        Value call(unsigned n) const
        {
            return {double(n)};
        }
    };
    static Unary_Array_Op<Scalar_Op> array_op;
    Value call(Frame& args) override
    {
        return array_op.op(Scalar_Op(*this, args), args[0]);
    }
};
struct Nat_To_Bool32_Function : public Legacy_Function
{
    static const char* name() { return "nat_to_bool32"; }
    Nat_To_Bool32_Function() : Legacy_Function(1,name()) {}
    struct Scalar_Op : public Unary_Num_Op
    {
        using Unary_Num_Op::Unary_Num_Op;
        Value call(double n) const
        {
            return {nat_to_bool32(num_to_nat(n, cx))};
        }
    };
    static Unary_Array_Op<Scalar_Op> array_op;
    Value call(Frame& args) override
    {
        return array_op.op(Scalar_Op(*this, args), args[0]);
    }
};

// Generalized dot product that includes vector dot product and matrix product.
// Same as Mathematica Dot[A,B]. Like APL A+.×B, Python numpy.dot(A,B)
struct Dot_Function : public Legacy_Function
{
    static const char* name() { return "dot"; }
    Dot_Function() : Legacy_Function(2,name()) {}
    Value call(Frame& args) override
    {
        return dot(args[0], args[1], At_Arg(*this, args));
    }
    SC_Value sc_call(SC_Frame& f) const override
    {
        auto a = f[0];
        auto b = f[1];
        if (!a.type.is_vec())
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
    static const char* name() { return "mag"; }
    Mag_Function() : Legacy_Function(1,name()) {}
    Value call(Frame& args) override
    {
        // TODO: use hypot() or BLAS DNRM2 or Eigen stableNorm/blueNorm?
        // Avoids overflow/underflow due to squaring of large/small values.
        // Slower.  https://forum.kde.org/viewtopic.php?f=74&t=62402
        auto list = args[0].to<List>(At_Arg(*this, args));
        // Fast path: assume we have a list of number, compute a result.
        double sum = 0.0;
        for (auto val : *list) {
            double x = val.get_num_or_nan();
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
                rlist->at(i) = r->expr(*arg_part(args.call_phrase_));
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
    SC_Value sc_call(SC_Frame& f) const override
    {
        auto arg = f[0];
        if (!arg.type.is_vec())
            throw Exception(At_SC_Arg(0, f), "mag: argument is not a vector");
        auto result = f.sc_.newvalue(SC_Type::Num());
        f.sc_.out() << "  float "<<result<<" = length("<<arg<<");\n";
        return result;
    }
};

struct Count_Function : public Legacy_Function
{
    static const char* name() { return "count"; }
    Count_Function() : Legacy_Function(1,name()) {}
    Value call(Frame& args) override
    {
        if (auto list = args[0].dycast<const List>())
            return {double(list->size())};
        if (auto string = args[0].dycast<const String>())
            return {double(string->size())};
        if (auto re = args[0].dycast<const Reactive_Value>()) {
            if (re->sctype_.is_list())
                return {double(re->sctype_.count())};
            //TODO:
            //if (re->sctype_ == SC_Type::Any())
        }
        throw Exception(At_Arg(*this, args), "not a list or string");
    }
    SC_Value sc_call(SC_Frame& f) const override
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
    static const char* name() { return "fields"; }
    Fields_Function() : Legacy_Function(1,name()) {}
    Value call(Frame& args) override
    {
        if (auto record = args[0].dycast<const Record>())
            return {record->fields()};
        throw Exception(At_Arg(*this, args), "not a record");
    }
};

struct Strcat_Function : public Legacy_Function
{
    static const char* name() { return "strcat"; }
    Strcat_Function() : Legacy_Function(1,name()) {}
    Value call(Frame& args) override
    {
        if (auto list = args[0].dycast<const List>()) {
            String_Builder sb;
            for (auto val : *list) {
                if (auto str = val.dycast<const String_or_Symbol>())
                    sb << *str;
                else if (val.is_bool())
                    sb << (val.get_bool_unsafe() ? "true" : "false");
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
    static const char* name() { return "repr"; }
    Repr_Function() : Legacy_Function(1,name()) {}
    Value call(Frame& args) override
    {
        String_Builder sb;
        sb << args[0];
        return {sb.get_string()};
    }
};
struct Decode_Function : public Legacy_Function
{
    static const char* name() { return "decode"; }
    Decode_Function() : Legacy_Function(1,name()) {}
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
    static const char* name() { return "encode"; }
    Encode_Function() : Legacy_Function(1,name()) {}
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
    static const char* name() { return "match"; }
    Match_Function() : Legacy_Function(1,name()) {}
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
    #define FUNCTION(f) {make_symbol(f::name()), make<Builtin_Value>(Value{make<f>()})}

    static const Namespace names = {
    {make_symbol("pi"), make<Builtin_Value>(pi)},
    {make_symbol("tau"), make<Builtin_Value>(two_pi)},
    {make_symbol("inf"), make<Builtin_Value>(INFINITY)},
    {make_symbol("false"), make<Builtin_Value>(Value(false))},
    {make_symbol("true"), make<Builtin_Value>(Value(true))},

    FUNCTION(Is_Bool_Function),
    FUNCTION(Is_Symbol_Function),
    FUNCTION(Is_Num_Function),
    FUNCTION(Is_String_Function),
    FUNCTION(Is_List_Function),
    FUNCTION(Is_Record_Function),
    FUNCTION(Is_Fun_Function),
    FUNCTION(Bit_Function),
    FUNCTION(Sqrt_Function),
    FUNCTION(Log_Function),
    FUNCTION(Abs_Function),
    FUNCTION(Floor_Function),
    FUNCTION(Ceil_Function),
    FUNCTION(Trunc_Function),
    FUNCTION(Round_Function),
    FUNCTION(Frac_Function),
    FUNCTION(Fhash_Function),
    FUNCTION(Sin_Function),
    FUNCTION(Cos_Function),
    FUNCTION(Tan_Function),
    FUNCTION(Asin_Function),
    FUNCTION(Acos_Function),
    FUNCTION(Atan_Function),
    FUNCTION(Atan2_Function),
    FUNCTION(Sinh_Function),
    FUNCTION(Cosh_Function),
    FUNCTION(Tanh_Function),
    FUNCTION(Asinh_Function),
    FUNCTION(Acosh_Function),
    FUNCTION(Atanh_Function),
    FUNCTION(Max_Function),
    FUNCTION(Min_Function),
    FUNCTION(And_Function),
    FUNCTION(Or_Function),
    FUNCTION(Xor_Function),
    FUNCTION(Lshift_Function),
    FUNCTION(Rshift_Function),
    FUNCTION(Bool32_Add_Function),
    FUNCTION(Bool32_To_Nat_Function),
    FUNCTION(Nat_To_Bool32_Function),
    FUNCTION(Dot_Function),
    FUNCTION(Mag_Function),
    FUNCTION(Count_Function),
    FUNCTION(Fields_Function),
    FUNCTION(Strcat_Function),
    FUNCTION(Repr_Function),
    FUNCTION(Decode_Function),
    FUNCTION(Encode_Function),
    FUNCTION(Match_Function),

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
