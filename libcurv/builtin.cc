// Copyright 2016-2021 Doug Moen
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
#include <libcurv/tree.h>
#include <libcurv/num.h>
#include <libcurv/pattern.h>
#include <libcurv/picker.h>
#include <libcurv/prim_expr.h>
#include <libcurv/program.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/sc_context.h>
#include <libcurv/source.h>
#include <libcurv/system.h>
#include <libcurv/types.h>

#include <boost/math/constants/constants.hpp>

#include <cassert>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <filesystem>
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
    Value call(Value arg, Fail fl, Frame& fm) const override
    {
        return Op::call(fl, At_Arg(*this, fm), arg);
    }
    SC_Value sc_call_expr(Operation& argx, Shared<const Phrase> ph, SC_Frame& fm)
    const override
    {
        return Op::sc_op(At_SC_Arg_Expr(*this, ph, fm), argx, fm);
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
    SC_Value sc_call_expr(Operation& argx, Shared<const Phrase> ph, SC_Frame& fm)
    const override
    {
        return Op::sc_op(At_SC_Arg_Expr(*this, ph, fm), argx, fm);
    }
};

template <class Prim>
struct Monoid_Func final : public Function
{
    using Function::Function;
    using Op = Binary_Array_Op<Prim>;
    Value call(Value arg, Fail fl, Frame& fm) const override
    {
        return Op::reduce(fl, At_Arg(*this, fm), Prim::zero(), arg);
    }
    SC_Value sc_call_expr(Operation& argx, Shared<const Phrase> ph, SC_Frame& fm)
    const override
    {
        return Op::sc_reduce(At_SC_Arg_Expr(*this, ph, fm),
            Prim::zero(), argx, fm);
    }
};

//-------------------//
// Builtin Functions //
//-------------------//

struct F_as : public Curried_Function
{
    F_as(const char* nm) : Curried_Function(2,nm) {}
    virtual Value ccall(const Function& self, Fail fl, Frame& args)
    const override {
        At_Arg cx(*this, args);
        auto type = value_to_type(args[0], Fail::hard, cx);
        if (type->contains(args[1], cx))
            return args[1];
        if (fl == Fail::soft)
            return missing;
        throw Exception(At_Arg(self, args),
            stringify(args[1]," is not a ",*type));
    }
    virtual bool validate_arg(unsigned i, Value a, Fail fl, const Context& cx)
    const override {
        return value_to_type(a, fl, cx) != nullptr;
    }
};
struct F_is : public Curried_Function
{
    F_is(const char* nm) : Curried_Function(2,nm) {}
    virtual Value ccall(const Function& self, Fail, Frame& args)
    const override {
        At_Arg cx(*this, args);
        auto type = value_to_type(args[0], Fail::hard, cx);
        return type->contains(args[1], cx);
    }
    virtual bool validate_arg(unsigned i, Value a, Fail fl, const Context& cx)
    const override {
        return value_to_type(a, fl, cx) != nullptr;
    }
};
struct F_Tuple : public Function
{
    using Function::Function;
    Value call(Value arg, Fail fl, Frame& fm) const override
    {
        std::vector<CType> types;
        TRY_DEF(list, arg.to<const List>(fl, At_Arg(*this, fm)));
        for (auto e : *list) {
            TRY_DEF(type, CType::from_value(e, fl, At_Arg(*this, fm)));
            types.push_back(type);
        }
        if (types.size() > 0) {
            // Tuple[T]=>Array[1]T, Tuple[T,T]=>Array[2]T, ...
            bool uniform = true;
            for (unsigned i = 1; i < types.size(); ++i) {
                if (types[0] != types[i]) {
                    uniform = false;
                    break;
                }
            }
            if (uniform)
                return {make<Array_Type>(types.size(), types[0])};
        }
        return {make<Tuple_Type>(std::move(types))};
    }
};
struct F_List : public Function
{
    using Function::Function;
    Value call(Value arg, Fail fl, Frame& fm) const override
    {
        TRY_DEF(type, CType::from_value(arg, fl, At_Arg(*this, fm)));
        return {make<List_Type>(type)};
    }
};
struct F_Array : public Curried_Function
{
    static CType make_array_type(
        unsigned i,
        const std::vector<int>& axes,
        CType etype)
    {
        if (i == axes.size()) return etype;
        /* otherwise i < axes.size() */
        if (axes[i] == 0)
            return {make<Tuple_Type>(std::vector<CType>{})};
        etype = make_array_type(i+1, axes, etype);
        return {make<Array_Type>(axes[i], etype)};
    }
    F_Array(const char* nm) : Curried_Function(2,nm) {}
    virtual Value ccall(const Function& self, Fail fl, Frame& args) const
    {
        At_Arg cx(*this, args);
        TRY_DEF(xlist, args[0].to<const List>(fl, cx));
        TRY_DEF(etype, CType::from_value(args[1], fl, cx));
        std::vector<int> axes;
        for (auto e : *xlist) {
            if (!e.is_num()) {
                FAIL(fl, false, cx, stringify(e," is not a number"));
            }
            int i;
            if (!num_to_int(e.to_num_unsafe(), i, 0, INT_MAX, fl, cx))
                return missing;
            axes.push_back(i);
        }
        return make_array_type(0, axes, etype).to_value();
    }
    virtual bool validate_arg(unsigned i, Value a, Fail fl, const Context& cx)
    const override {
        auto xlist = a.to<const List>(fl, cx);
        if (xlist == nullptr) return false;
        for (auto e : *xlist) {
            if (!e.is_num()) {
                FAIL(fl, false, cx, stringify(e," is not a number"));
            }
            int i;
            if (!num_to_int(e.to_num_unsafe(), i, 0, INT_MAX, fl, cx))
                return false;
        }
        return true;
    }
};
struct F_Struct : public Function
{
    using Function::Function;
    Value call(Value arg, Fail fl, Frame& fm) const override
    {
        Symbol_Map<CType> fields;
        At_Arg cx(*this, fm);
        TRY_DEF(rec, arg.to<const Record>(fl, cx));
        for (auto pf = rec->iter(); !pf->empty(); pf->next()) {
            TRY_DEF(type, CType::from_value(pf->value(cx), fl, cx));
            fields[pf->key()] = type;
        }
        return {make<Struct_Type>(std::move(fields))};
    }
};
struct F_Record : public Function
{
    using Function::Function;
    Value call(Value arg, Fail fl, Frame& fm) const override
    {
        Symbol_Map<CType> fields;
        At_Arg cx(*this, fm);
        TRY_DEF(rec, arg.to<const Record>(fl, cx));
        for (auto pf = rec->iter(); !pf->empty(); pf->next()) {
            TRY_DEF(type, CType::from_value(pf->value(cx), fl, cx));
            fields[pf->key()] = type;
        }
        return {make<Record_Type>(std::move(fields))};
    }
};
struct F_is_bool : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {is_bool(arg)};
    }
};
struct F_is_char : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {arg.is_char()};
    }
};
struct F_is_symbol : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {is_symbol(arg)};
    }
};
struct F_is_num : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {is_num(arg)};
    }
};
struct F_is_string : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {is_string(arg)};
    }
};
struct F_is_list : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame& fm) const override
    {
        Generic_List glist(arg);
        return {glist.is_list()};
    }
};
struct F_is_record : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {arg.maybe<Record>() != nullptr};
    }
};
struct F_is_primitive_func : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame&) const override
    {
        return {arg.maybe<Function>() != nullptr};
    }
};
struct F_is_func : public Function
{
    using Function::Function;
    Value call(Value arg, Fail, Frame& fm) const override
    {
        return {maybe_function(arg, At_Arg(*this, fm)) != nullptr};
    }
};

struct Bit_Prim : public Unary_Bool_To_Num_Prim
{
    static const char* name() { return "bit"; }
    static constexpr Prec prec = Prec::postfix;
    static Value call(bool b, const Context&) { return {double(b)}; }
    static SC_Value sc_call(SC_Frame& fm, SC_Value arg)
    {
        auto result = fm.sc_.newvalue(SC_Type::Num(arg.type.count()));
        fm.sc_.out() << "  " << result.type << " " << result << " = "
            << result.type << "(" << arg << ");\n";
        return result;
    }
};
using F_bit = Unary_Array_Func<Bit_Prim>;

#define UNARY_NUMERIC_FUNCTION(Func_Name,curv_name,c_name,glsl_name) \
struct Func_Name##Prim : public Unary_Num_SCVec_Prim \
{ \
    static const char* name() { return #curv_name; } \
    static constexpr Prec prec = Prec::postfix; \
    static Value call(double x, const Context&) { return {c_name(x)}; } \
    static SC_Value sc_call(SC_Frame& fm, SC_Value arg) \
        { return sc_unary_call(fm, arg.type, #glsl_name, arg); } \
}; \
using Func_Name = Unary_Array_Func<Func_Name##Prim>; \

UNARY_NUMERIC_FUNCTION(F_sqrt, sqrt, sqrt, sqrt)
UNARY_NUMERIC_FUNCTION(F_log, log, log, log)
UNARY_NUMERIC_FUNCTION(F_abs, abs, abs, abs)
UNARY_NUMERIC_FUNCTION(F_floor, floor, floor, floor)
UNARY_NUMERIC_FUNCTION(F_ceil, ceil, ceil, ceil)
UNARY_NUMERIC_FUNCTION(F_trunc, trunc, trunc, trunc)
UNARY_NUMERIC_FUNCTION(F_round, round, rint, roundEven)

inline double frac(double n) { return n - floor(n); }
UNARY_NUMERIC_FUNCTION(F_frac, frac, frac, fract)

inline double sign(double n) { return copysign(double(n!=0),n); }
UNARY_NUMERIC_FUNCTION(F_sign, sign, sign, sign)

UNARY_NUMERIC_FUNCTION(F_sin, sin, sin, sin)
UNARY_NUMERIC_FUNCTION(F_cos, cos, cos, cos)
UNARY_NUMERIC_FUNCTION(F_tan, tan, tan, tan)
UNARY_NUMERIC_FUNCTION(F_acos, acos, acos, acos)
UNARY_NUMERIC_FUNCTION(F_asin, asin, asin, asin)
UNARY_NUMERIC_FUNCTION(F_atan, atan, atan, atan)

UNARY_NUMERIC_FUNCTION(F_sinh, sinh, sinh, sinh)
UNARY_NUMERIC_FUNCTION(F_cosh, cosh, cosh, cosh)
UNARY_NUMERIC_FUNCTION(F_tanh, tanh, tanh, tanh)
UNARY_NUMERIC_FUNCTION(F_acosh, acosh, acosh, acosh)
UNARY_NUMERIC_FUNCTION(F_asinh, asinh, asinh, asinh)
UNARY_NUMERIC_FUNCTION(F_atanh, atanh, atanh, atanh)

struct Phase_Prim : public Unary_Vec2_To_Num_Prim
{
    static const char* name() { return "phase"; }
    static constexpr Prec prec = Prec::postfix;
    static Value call(Vec2 v, const Context&) { return {atan2(v.y,v.x)}; }
    static SC_Value sc_call(SC_Frame& fm, SC_Value arg) {
        auto result = fm.sc_.newvalue(SC_Type::Num());
        fm.sc_.out() << "  " << result.type << " " << result << " = "
            << "atan(" << arg << ".y," << arg << ".x);\n";
        return result;
    }
};
using F_phase = Unary_Array_Func<Phase_Prim>;

struct Max_Prim : public Binary_Num_SCVec_Prim
{
    static const char* name() { return "max"; }
    static constexpr Prec prec = Prec::postfix;
    static Value zero() { return {-INFINITY}; }
    static Value call(double x, double y, const Context&)
        { return {std::max(x,y)}; }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y)
        { return sc_bincall(fm, x.type, "max", x, y); }
};
using F_max = Monoid_Func<Max_Prim>;

struct Min_Prim : public Binary_Num_SCVec_Prim
{
    static const char* name() { return "min"; }
    static constexpr Prec prec = Prec::postfix;
    static Value zero() { return {INFINITY}; }
    static Value call(double x, double y, const Context&)
        { return {std::min(x,y)}; }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y)
        { return sc_bincall(fm, x.type, "min", x, y); }
};
using F_min = Monoid_Func<Min_Prim>;

using F_sum = Monoid_Func<Add_Prim>;

using F_not = Unary_Array_Func<Not_Prim>;

#define BOOL_OP(CppName,Name,Zero,LogOp,BitOp)\
struct CppName##_Prim : public Binary_Bool_Prim\
{\
    static const char* name() { return Name; } \
    static constexpr Prec prec = Prec::postfix; \
    static Value zero() { return {Zero}; }\
    static Value call(bool x, bool y, const Context&) { return {x LogOp y}; }\
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y)\
    {\
        auto result = fm.sc_.newvalue(x.type);\
        fm.sc_.out() << "  " << x.type << " " << result << " = ";\
        if (x.type.is_bool())\
            fm.sc_.out() << x << #LogOp << y << ";\n";\
        else if (x.type.is_bool_or_vec()) {\
            /* In GLSL 4.6, I *think* you can use '&' and '|' instead. */ \
            /* TODO: SubCurv: more efficient and|or in bvec case */ \
            bool first = true;\
            fm.sc_.out() << x.type << "(";\
            for (unsigned i = 0; i < x.type.count(); ++i) {\
                if (!first) fm.sc_.out() << ",";\
                first = false;\
                fm.sc_.out() << x << "[" << i << "]"\
                    << #LogOp << y << "[" << i << "]";\
            }\
            fm.sc_.out() << ")";\
        }\
        else\
            fm.sc_.out() << x << #BitOp << y << ";\n";\
        fm.sc_.out() << ";\n";\
        return result;\
    }\
};\
using F_##CppName = Monoid_Func<CppName##_Prim>;\

BOOL_OP(and,"and",true,&&,&)
BOOL_OP(or,"or",false,||,|)

struct Xor_Prim : public Binary_Bool_Prim
{
    static const char* name() { return "xor"; }
    static constexpr Prec prec = Prec::postfix;
    static Value zero() { return {false}; }
    static Value call(bool x, bool y, const Context&) { return {x != y}; }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y)
    {
        auto result = fm.sc_.newvalue(x.type);
        fm.sc_.out() << "  " << result.type << " " << result << " = ";
        if (x.type.is_bool())
            fm.sc_.out() << x << "!=" << y;
        else if (x.type.is_bool_or_vec())
            fm.sc_.out() << "notEqual(" << x << "," << y << ")";
        else // bool32 or vector of bool32
            fm.sc_.out() << x << "^" << y;
        fm.sc_.out() << ";\n";
        return result;
    }
};
using F_xor = Monoid_Func<Xor_Prim>;

struct Lshift_Prim : public Shift_Prim
{
    static const char* name() { return "lshift"; }
    static constexpr Prec prec = Prec::postfix;
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
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y)
    {
        auto result = fm.sc_.newvalue(x.type);
        fm.sc_.out() << "  " << x.type << " " << result << " = "
            << x << " << int(" << y << ");\n";
        return result;
    }
};
using F_lshift = Binary_Array_Func<Lshift_Prim>;

struct Rshift_Prim : public Shift_Prim
{
    static const char* name() { return "rshift"; }
    static constexpr Prec prec = Prec::postfix;
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
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y)
    {
        auto result = fm.sc_.newvalue(x.type);
        fm.sc_.out() << "  " << x.type << " " << result << " = "
            << x << " >> int(" << y << ");\n";
        return result;
    }
};
using F_rshift = Binary_Array_Func<Rshift_Prim>;

struct Bool32_Sum_Prim : public Binary_Bool32_Prim
{
    static const char* name() { return "bool32_sum"; }
    static constexpr Prec prec = Prec::postfix;
    static Value zero()
    {
        static Value z = {nat_to_bool32(0)};
        return z;
    }
    static Value call(unsigned a, unsigned b, const Context&)
    {
        return {nat_to_bool32(a + b)};
    }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y)
    {
        auto result = fm.sc_.newvalue(x.type);
        fm.sc_.out() << "  " << x.type << " " << result << " = "
            << x << " + " << y << ";\n";
        return result;
    }
};
using F_bool32_sum = Monoid_Func<Bool32_Sum_Prim>;

struct Bool32_Product_Prim : public Binary_Bool32_Prim
{
    static const char* name() { return "bool32_product"; }
    static constexpr Prec prec = Prec::postfix;
    static Value zero()
    {
        static Value z = {nat_to_bool32(1)};
        return z;
    }
    static Value call(unsigned a, unsigned b, const Context&)
    {
        return {nat_to_bool32(a * b)};
    }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x, SC_Value y)
    {
        auto result = fm.sc_.newvalue(x.type);
        fm.sc_.out() << "  " << x.type << " " << result << " = "
            << x << " * " << y << ";\n";
        return result;
    }
};
using F_bool32_product = Monoid_Func<Bool32_Product_Prim>;

struct F_bool32_to_nat : public Function
{
    using Function::Function;
    struct Prim : public Unary_Bool32_To_Num_Prim
    {
        static const char* name() { return "bool32_to_nat"; }
        static constexpr Prec prec = Prec::postfix;
        static Value call(unsigned n, const Context&)
        {
            return {double(n)};
        }
        // No SubCurv support because a Num (32 bit float)
        // cannot hold a Nat (32 bit natural).
        static SC_Type sc_result_type(SC_Type) { return {}; }
        static SC_Value sc_call(SC_Frame& fm, SC_Value x)
        {
            throw Exception(At_SC_Frame(fm),
                "bool32_to_nat is not supported by SubCurv");
        }
    };
    static Unary_Array_Op<Prim> array_op;
    Value call(Value arg, Fail fl, Frame& fm) const override
    {
        return array_op.call(fl, At_Arg(*this, fm), arg);
    }
};

struct F_nat_to_bool32 : public Function
{
    using Function::Function;
    struct Prim : public Unary_Num_To_Bool32_Prim
    {
        static const char* name() { return "nat_to_bool32"; }
        static constexpr Prec prec = Prec::postfix;
        static Value call(double n, const Context& cx)
        {
            return {nat_to_bool32(num_to_nat(n, cx))};
        }
        static SC_Value sc_call(SC_Frame& fm, SC_Value x)
        {
            throw Exception(At_SC_Frame(fm),
                "nat_to_bool32 can't be called in this context: "
                "argument must be a constant");
        }
    };
    static Unary_Array_Op<Prim> array_op;
    Value call(Value arg, Fail fl, Frame& fm) const override
    {
        return array_op.call(fl, At_Arg(*this, fm), arg);
    }
    SC_Value sc_call_expr(Operation& argx, Shared<const Phrase> ph, SC_Frame& fm)
    const override
    {
        At_SC_Arg_Expr cx(*this, ph, fm);
        if (auto k = dynamic_cast<const Constant*>(&argx)) {
            unsigned n = num_to_nat(k->value_.to_num(cx), cx);
            auto type = SC_Type::Bool32();
            auto result = fm.sc_.newvalue(type);
            fm.sc_.out() << "  " << type << " " << result << " = "
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
    static constexpr Prec prec = Prec::postfix;
    static Value call(unsigned n, const Context&)
    {
        return {bitcast_nat_to_float(n)};
    }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x)
    {
        unsigned count = x.type.is_bool32() ? 1 : x.type.count();
        auto result = fm.sc_.newvalue(SC_Type::Num(count));
        fm.sc_.out() << "  " << result.type << " " << result
            << " = uintBitsToFloat(" << x << ");\n";
        return result;
    }
};
using F_bool32_to_float = Unary_Array_Func<Bool32_To_Float_Prim>;

struct Float_To_Bool32_Prim : public Unary_Num_To_Bool32_Prim
{
    static const char* name() { return "bool32_to_float"; }
    static constexpr Prec prec = Prec::postfix;
    static Value call(double n, const Context&)
    {
        return {nat_to_bool32(bitcast_float_to_nat(n))};
    }
    static SC_Value sc_call(SC_Frame& fm, SC_Value x)
    {
        auto result = fm.sc_.newvalue(SC_Type::Bool32(x.type.count()));
        fm.sc_.out() << "  " << result.type << " " << result
            << " = floatBitsToUint(" << x << ");\n";
        return result;
    }
};
using F_float_to_bool32 = Unary_Array_Func<Float_To_Bool32_Prim>;

Value
select(Value a, Value b, Value c, Fail fl, const Context& cx)
{
    if (a.is_bool())
        return a.to_bool_unsafe() ? b : c;
    if (auto alist = a.maybe<List>()) {
        auto blist = b.maybe<Abstract_List>();
        if (blist) {
            ASSERT_SIZE(fl,missing,blist,alist->size(),At_Index(1, cx));
        }
        auto clist = c.maybe<Abstract_List>();
        if (clist) {
            ASSERT_SIZE(fl,missing,clist,alist->size(),At_Index(2, cx));
        }
        List_Builder lb;
        for (unsigned i = 0; i < alist->size(); ++i) {
            TRY_DEF(v, select(alist->at(i),
                              blist ? blist->val_at(i) : b,
                              clist ? clist->val_at(i) : c,
                              fl, cx));
            lb.push_back(v);
        }
        return lb.get_value();
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
struct F_select : public Tuple_Function
{
    F_select(const char* nm) : Tuple_Function(3,nm) {}
    Value tuple_call(Fail fl, Frame& args) const override
    {
        return select(args[0], args[1], args[2], fl, At_Arg(*this, args));
    }
    SC_Value sc_tuple_call(SC_Frame& fm) const override
    {
        auto cond = fm[0];
        auto consequent = fm[1];
        auto alternate = fm[2];
        if (!cond.type.is_bool_or_vec()) {
            throw Exception(At_SC_Tuple_Arg(0,fm), stringify(
                "argument is not bool or bool vector; it has type ",
                cond.type));
        }
        if (consequent.type != alternate.type) {
            throw Exception(At_SC_Tuple_Arg(1,fm), stringify(
                "2nd and 3rd argument of 'select' have different types: ",
                consequent.type, " and ", alternate.type));
        }
        SC_Value result;
        if (cond.type.is_bool()) {
            result = fm.sc_.newvalue(consequent.type);
            fm.sc_.out() << "  " << result.type << " " << result << " = ";
            fm.sc_.out() << cond << "?" << consequent << ":" << alternate;
        } else {
            // 'cond' is a boolean vector.
            if (consequent.type.count() == 1) {
                // Consequent & alternate are scalars. Convert them to vectors.
                auto T = SC_Type::Array(consequent.type, cond.type.count());
                sc_try_extend(fm, consequent, T);
                sc_try_extend(fm, alternate, T);
            }
            else if (!consequent.type.is_vec()) {
                throw Exception(At_SC_Tuple_Arg(1,fm), stringify(
                    "Must be a scalar or vector to match condition argument."
                    " Instead, type is ", consequent.type));
            }
            else if (cond.type.count() != consequent.type.count()) {
                throw Exception(At_SC_Tuple_Arg(1,fm), stringify(
                    "Vector length ",consequent.type.count()," does not match"
                    " length of condition vector (", cond.type.count(),")"));
            }
            result = fm.sc_.newvalue(consequent.type);
            fm.sc_.out() << "  " << result.type << " " << result << " = ";
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
                fm.sc_.out() << "mix(" << alternate << "," << consequent
                    << ",vec" << cond.type.count() << "(" << cond << "))";
            } else {
                fm.sc_.out() << result.type << "(";
                bool atfirst = true;
                for (unsigned i = 0; i < result.type.count(); ++i) {
                    if (!atfirst) fm.sc_.out() << ",";
                    atfirst = false;
                    fm.sc_.out() << cond << "[" << i << "] ? "
                                << consequent << "[" << i << "] : "
                                << alternate << "[" << i << "]";
                }
                fm.sc_.out() << ")";
            }
        }
        fm.sc_.out() << ";\n";
        return result;
    }
};

SC_Value Equal_Expr::sc_eval(SC_Frame& fm) const
{
    auto a = sc_eval_op(fm, *arg1_);
    auto b = sc_eval_op(fm, *arg2_);
    if (a.type != b.type || a.type.plex_array_rank() > 0) {
        throw Exception(At_SC_Phrase(syntax_, fm),
            stringify("domain error: ",a.type," == ",b.type));
    }
    SC_Value result = fm.sc_.newvalue(SC_Type::Bool());
    fm.sc_.out() <<"  bool "<<result<<" =("<<a<<" == "<<b<<");\n";
    return result;
}
SC_Value Not_Equal_Expr::sc_eval(SC_Frame& fm) const
{
    auto a = sc_eval_op(fm, *arg1_);
    auto b = sc_eval_op(fm, *arg2_);
    if (a.type != b.type || a.type.plex_array_rank() > 0) {
        throw Exception(At_SC_Phrase(syntax_, fm),
            stringify("domain error: ",a.type," != ",b.type));
    }
    SC_Value result = fm.sc_.newvalue(SC_Type::Bool());
    fm.sc_.out() <<"  bool "<<result<<" =("<<a<<" != "<<b<<");\n";
    return result;
}

// Generalized dot product that includes vector dot product and matrix product.
// Same as Mathematica Dot[A,B]. Like APL A+.×B, Python numpy.dot(A,B)
//  dot(a,b) =
//    if (count a > 0 && is_list(a[0]))
//      [for (row in a) dot(row,b)]  // matrix*...
//    else
//      sum(a*b)                     // vector*...
struct F_dot : public Tuple_Function
{
    F_dot(const char* nm) : Tuple_Function(2,nm) {}
    Value dot(Value a, Value b, Fail, const At_Arg& cx) const;
    Value tuple_call(Fail fl, Frame& args) const override
    {
        return dot(args[0], args[1], fl, At_Arg(*this, args));
    }
    SC_Value sc_tuple_call(SC_Frame& fm) const override
    {
        auto a = fm[0];
        auto b = fm[1];
        if (a.type.is_num_vec() && a.type == b.type)
            return sc_bincall(fm, SC_Type::Num(), "dot", a, b);
        if (a.type.is_num_vec() && b.type.is_mat()
            && a.type.count() == b.type.count())
        {
            return sc_binop(fm, a.type, b, "*", a);
        }
        if (a.type.is_mat() && b.type.is_num_vec()
            && a.type.count() == b.type.count())
        {
            return sc_binop(fm, b.type, b, "*", a);
        }
        if (a.type.is_mat() && b.type.is_mat()
            && a.type.count() == b.type.count())
        {
            return sc_binop(fm, a.type, b, "*", a);
        }
        throw Exception(At_SC_Frame(fm), stringify(
            "dot: invalid argument type [",a.type,",",b.type,"]"));
    }
};
Value F_dot::dot(Value a, Value b, Fail fl, const At_Arg& cx) const
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
    //   This is copied and modified from F_dot::sc_tuple_call.
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

struct F_mag : public Tuple_Function
{
    F_mag(const char* nm) : Tuple_Function(1,nm) {}
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
    SC_Value sc_tuple_call(SC_Frame& fm) const override
    {
        auto arg = fm[0];
        if (!arg.type.is_num_vec())
            throw Exception(At_SC_Tuple_Arg(0, fm), stringify(
                "mag: argument is not a vector (type ", arg.type, ")"));
        auto result = fm.sc_.newvalue(SC_Type::Num());
        fm.sc_.out() << "  float "<<result<<" = length("<<arg<<");\n";
        return result;
    }
};

struct F_count : public Tuple_Function
{
    F_count(const char* nm) : Tuple_Function(1,nm) {}
    Value tuple_call(Fail fl, Frame& args) const override
    {
        if (auto list = args[0].maybe<const Abstract_List>())
            return {double(list->size())};
        if (auto re = args[0].maybe<const Reactive_Value>()) {
            if (re->sctype_.is_array())
                return {double(re->sctype_.count())};
        }
        FAIL(fl, missing, At_Arg(*this, args), "not a list or string");
    }
    SC_Value sc_tuple_call(SC_Frame& fm) const override
    {
        auto arg = fm[0];
        if (!arg.type.is_array())
            throw Exception(At_SC_Tuple_Arg(0, fm), stringify(
                "count: argument is not a list (type ",arg.type,")"));
        auto result = fm.sc_.newvalue(SC_Type::Num());
        fm.sc_.out() << "  float "<<result<<" = "<<arg.type.count()<<";\n";
        return result;
    }
};
struct F_fields : public Tuple_Function
{
    F_fields(const char* nm) : Tuple_Function(1,nm) {}
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

// Construct a character from an integer or a string of length 1.
// Vectorized.
Value to_char(Value arg, Fail fl, const Context& cx)
{
    if (arg.is_num()) {
        int code;
        if (!num_to_int(arg.to_num_unsafe(), code, 1, 127, fl, cx))
            return missing;
        return Value{char(code)};
    }
    else if (auto list = arg.maybe<List>()) {
        if (list->empty()) return arg;
        Shared<String> s = make_uninitialized_string(list->size());
        for (unsigned i = 0; i < list->size(); ++i) {
            TRY_DEF(val, to_char(list->at(i), fl, cx));
            if (val.is_char())
                s->at(i) = val.to_char_unsafe();
            else {
                Shared<List> result = List::make(list->size());
                for (unsigned j = 0; j < i; ++j)
                    result->at(j) = Value(s->at(j));
                result->at(i) = val;
                for (unsigned k = i+1; k < list->size(); ++k)
                    result->at(i) = to_char(list->at(i), fl, cx);
                return {result};
            }
        }
        return {s};
    }
    else {
        FAIL(fl, missing, cx,
            stringify(arg, " is not an integer, or a list or tree of integers"));
    }
}
struct F_char : public Function
{
    using Function::Function;
    virtual Value call(Value arg, Fail fl, Frame& fm) const override
    {
        return to_char(arg, fl, At_Arg(*this, fm));
    }
};
Value ucode(Value arg, Fail fl, const Context& cx)
{
    if (arg.is_char())
        return {(double)(unsigned)arg.to_char_unsafe()};
    if (auto str = arg.maybe<const String>()) {
        List_Builder lb;
        for (size_t i = 0; i < str->size(); ++i)
            lb.push_back({(double)(unsigned)str->at(i)});
        return lb.get_value();
    }
    if (auto list = arg.maybe<const List>()) {
        List_Builder lb;
        for (Value e : *list) {
            TRY_DEF(r, ucode(e, fl, cx));
            lb.push_back(r);
        }
        return lb.get_value();
    }
    FAIL(fl, missing, cx,
        stringify(arg, " is not a character or list of characters"));
}
struct F_ucode : public Function
{
    using Function::Function;
    virtual Value call(Value arg, Fail fl, Frame& fm) const override
    {
        return ucode(arg, fl, At_Arg(*this, fm));
    }
};
struct F_symbol : public Function
{
    using Function::Function;
    virtual Value call(Value arg, Fail fl, Frame& fm) const override
    {
        At_Arg cx(*this, fm);
        TRY_DEF(string, value_to_string(arg, fl, cx));
        for (auto c : *string) {
            if (c <= ' ' || c >= '~') {
                FAIL(fl,missing,cx, stringify(
                    "string ",arg," contains ",illegal_character_message(c)));
            }
        }
        auto symbol = make_symbol(string->data(), string->size());
        return symbol.to_value();
    }
};
struct F_string : public Function
{
    using Function::Function;
    virtual Value call(Value arg, Fail fl, Frame& fm) const override
    {
        String_Builder sb;
        arg.print_string(sb);
        return sb.get_value();
    }
};
struct F_repr : public Tuple_Function
{
    F_repr(const char* nm) : Tuple_Function(1,nm) {}
    Value tuple_call(Fail, Frame& args) const override
    {
        String_Builder sb;
        sb << args[0];
        return sb.get_value();
    }
};

struct F_match : public Function
{
    using Function::Function;
    virtual Value call(Value arg, Fail fl, Frame& fm) const override
    {
        At_Arg ctx0(*this, fm);
        TRY_DEF(list, arg.to<List>(fl, ctx0));
        std::vector<Shared<const Function>> cases;
        for (size_t i = 0; i < list->size(); ++i) {
            TRY_DEF(fn, value_to_function(list->at(i), fl, At_Index(i,ctx0)));
            cases.push_back(fn);
        }
        auto mf = make<Piecewise_Function>(cases);
        mf->fname_.name_ = fname_.name_;
        mf->fname_.argpos_ = 1;
        return {mf};
    }
};

struct F_compose : public Function
{
    using Function::Function;
    virtual Value call(Value arg, Fail fl, Frame& fm) const override
    {
        At_Arg ctx0(*this, fm);
        TRY_DEF(list, arg.to<List>(fl, ctx0));
        std::vector<Shared<const Function>> cases;
        for (size_t i = 0; i < list->size(); ++i) {
            TRY_DEF(fn, value_to_function(list->at(i), fl, At_Index(i,ctx0)));
            cases.push_back(fn);
        }
        auto mf = make<Composite_Function>(cases);
        mf->fname_.name_ = fname_.name_;
        mf->fname_.argpos_ = 1;
        return {mf};
    }
};

struct F_tslice : public Function
{
    using Function::Function;
    virtual Value call(Value arg, Fail fl, Frame& fm) const override
    {
        At_Arg cx(*this, fm);
        TRY_DEF(list, arg.to<List>(fl, cx));
        return make_tslice(list->begin(), list->end());
    }
};
struct F_tpath : public Function
{
    using Function::Function;
    virtual Value call(Value arg, Fail fl, Frame& fm) const override
    {
        At_Arg cx(*this, fm);
        TRY_DEF(list, arg.to<List>(fl, cx));
        return make_tpath(list->begin(), list->end());
    }
};
struct F_amend : public Curried_Function
{
    F_amend(const char* nm) : Curried_Function(3,nm) {}
    virtual Value ccall(const Function& self, Fail, Frame& args) const
    {
        return tree_amend(args[2], args[0], args[1], At_Arg(*this, args));
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
        Just_Expression(move(src)),
        arg_(move(arg))
    {}
    virtual Value eval(Frame& fm) const override
    {
        // Each call to `file pathname` has its own stack frame,
        // which permits calls to `file pathname` to appear in stack traces.
        auto& callphrase = dynamic_cast<const Call_Phrase&>(*syntax_);
        std::unique_ptr<Frame> f2 = make_tail_array<Frame>(0,
            fm.sstate_, &fm, fm.func_, &callphrase);
        At_Metacall_With_Call_Frame cx("file", 0, *f2);

        // construct file pathname from argument
        Value arg = arg_->eval(fm);
        auto argstr = value_to_string(arg, Fail::hard, cx);
        namespace fs = std::filesystem;
        fs::path filepath;
        auto caller_filename = syntax_->location().source().name_;
        if (caller_filename->empty()) {
            filepath = fs::path(argstr->c_str());
        } else {
            filepath = fs::path(caller_filename->c_str()).parent_path()
                / fs::path(argstr->c_str());
        }

        return import_value(import, filepath, cx);
    }
};
struct File_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        return make<File_Expr>(share(ph), analyse_op(*ph.arg_, env));
    }
    virtual void print_help(std::ostream& out) const override
    {
        out <<
            "file <filename>\n"
            "  Evaluate the program stored in the file named <filename>, and return the resulting value.\n"
            "  <filename> is a string.\n";
    }
};

/// The meaning of a call to `print`, such as `print "foo"`.
struct Print_Action : public Just_Action
{
    Shared<Operation> arg_;
    Print_Action(
        Shared<const Phrase> syntax,
        Shared<Operation> arg)
    :
        Just_Action(move(syntax)),
        arg_(move(arg))
    {}
    virtual void exec(Frame& fm, Executor&) const override
    {
        Value arg = arg_->eval(fm);
        auto str = to_print_string(arg);
        fm.sstate_.system_.print(str->c_str());
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
    virtual void print_help(std::ostream& out) const override
    {
        out <<
            "print <message>\n"
            "  Print a message string on the debug console, followed by newline. If <message> is not a string,\n"
            "  it is converted to a string using the 'string' function.\n";
    }
};

struct Warning_Action : public Just_Action
{
    Shared<Operation> arg_;
    Warning_Action(
        Shared<const Phrase> syntax,
        Shared<Operation> arg)
    :
        Just_Action(move(syntax)),
        arg_(move(arg))
    {}
    virtual void exec(Frame& fm, Executor&) const override
    {
        Value arg = arg_->eval(fm);
        auto msg = to_print_string(arg);
        Exception exc{At_Phrase(*syntax_, fm), msg};
        fm.sstate_.system_.warning(exc);
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
    virtual void print_help(std::ostream& out) const override
    {
        out <<
            "warning <message>\n"
            "  Print a message string on the debug console, preceded by 'WARNING: ',\n"
            "  followed by newline and then a stack trace. If <message> is not a string,\n"
            "  it is converted to a string using the 'string' function.\n";
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
struct Error_Operation : public Just_Action
{
    Shared<Operation> arg_;
    Error_Operation(
        Shared<const Phrase> syntax,
        Shared<Operation> arg)
    :
        Just_Action(move(syntax)),
        arg_(move(arg))
    {}
    [[noreturn]] void run(Frame& fm) const
    {
        Value val = arg_->eval(fm);
        auto msg = to_print_string(val);
        throw Exception{At_Phrase(*syntax_, fm), msg};
    }
    virtual void exec(Frame& fm, Executor&) const override
    {
        run(fm);
    }
    virtual Value eval(Frame& fm) const override
    {
        run(fm);
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
    virtual Shared<Operation> to_operation(Source_State&) override
    {
        return make<Constant>(syntax_, Value{make<Error_Function>("error")});
    }
    virtual void print_help(std::ostream& out) const override
    {
        out <<
            "error <message>\n"
            "  On the debug console, print 'ERROR: ', then the message string, then newline and a stack trace.\n"
            "  Then terminate the program. If <message> is not a string,\n"
            "  convert it to a string using the 'string' function.\n";
    }
};

// exec(expr) -- a debug action that evaluates expr, then discards the result.
// It is used to call functions or source files for their side effects.
struct Exec_Action : public Just_Action
{
    Shared<Operation> arg_;
    Exec_Action(
        Shared<const Phrase> syntax,
        Shared<Operation> arg)
    :
        Just_Action(move(syntax)),
        arg_(move(arg))
    {}
    virtual void exec(Frame& fm, Executor&) const override
    {
        arg_->eval(fm);
    }
};
struct Exec_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        return make<Exec_Action>(share(ph), analyse_op(*ph.arg_, env));
    }
    virtual void print_help(std::ostream& out) const override
    {
        out <<
            "exec <expression>\n"
            "  Evaluate the expression and then ignore the result. This is used for calling a function whose only\n"
            "  purpose is to have a side effect (by executing debug statements) and you don't care about the result.\n";
    }
};

struct Assert_Action : public Just_Action
{
    Shared<Operation> arg_;
    Assert_Action(
        Shared<const Phrase> syntax,
        Shared<Operation> arg)
    :
        Just_Action(move(syntax)),
        arg_(move(arg))
    {}
    virtual void exec(Frame& fm, Executor&) const override
    {
        At_Metacall cx{"assert", 0, *arg_->syntax_, fm};
        bool b = arg_->eval(fm).to_bool(cx);
        if (!b)
            throw Exception(At_Phrase(*syntax_, fm), "assertion failed");
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
    virtual void print_help(std::ostream& out) const override
    {
        out <<
            "assert <condition>\n"
            "  Evaluate the condition, which must be true or false. If it is true, then nothing happens.\n"
            "  If it is false, then an assertion failure error message is produced, followed by a stack trace,\n"
            "  and the program is terminated.\n";
    }
};

struct Assert_Error_Action : public Just_Action
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
        Just_Action(move(syntax)),
        expected_message_(move(expected_message)),
        actual_message_(move(actual_message)),
        expr_(move(expr))
    {}

    virtual void exec(Frame& fm, Executor&) const override
    {
        Value expected_msg_val = expected_message_->eval(fm);
        auto expected_msg_str =
            value_to_string(expected_msg_val, Fail::hard,
                At_Phrase(*expected_message_->syntax_, fm));

        if (actual_message_ != nullptr) {
            if (*actual_message_ != *expected_msg_str)
                throw Exception(At_Phrase(*syntax_, fm),
                    stringify("assertion failed: expected error \"",
                        expected_msg_str,
                        "\", actual error \"",
                        actual_message_,
                        "\""));
            return;
        }

        Value result;
        try {
            result = expr_->eval(fm);
        } catch (Exception& e) {
            if (*e.shared_what() != *expected_msg_str) {
                throw Exception(At_Phrase(*syntax_, fm),
                    stringify("assertion failed: expected error \"",
                        expected_msg_str,
                        "\", actual error \"",
                        e.shared_what(),
                        "\""));
            }
            return;
        }
        throw Exception(At_Phrase(*syntax_, fm),
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
    virtual void print_help(std::ostream& out) const override
    {
        out <<
            "assert_error [error_message_string, expression]\n"
            "  Evaluate the expression argument. Assert that the expression evaluation terminates with an error,\n"
            "  and that the resulting error message is equal to error_message_string. Used for unit testing.\n";
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
        Just_Expression(move(syntax)),
        expr_(move(expr)),
        selector_(move(selector))
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
    virtual Value eval(Frame& fm) const override
    {
        auto val = expr_->eval(fm);
        auto id = selector_.eval(fm);
        return defined_at(val, id);
    }
};
struct Defined_Metafunction : public Metafunction
{
    using Metafunction::Metafunction;
    virtual Shared<Meaning> call(const Call_Phrase& ph, Environ& env) override
    {
        auto argph = strip_parens(ph.arg_);
        if (auto dot = cast<const Dot_Phrase>(argph)) {
            if (auto brackets = cast<const Bracket_Phrase>(dot->right_)) {
                return make<Defined_Expression>(
                    share(ph),
                    analyse_op(*dot->left_, env),
                    Symbol_Expr(analyse_op(*brackets->body_, env)));
            }
            if (auto string = cast<const String_Phrase>(dot->right_)) {
                env.sstate_.deprecate(
                    &Source_State::dot_string_deprecated_, 1,
                    At_Phrase(*argph, env),
                    Source_State::dot_string_deprecated_msg);
            }
        }
        auto arg = analyse_op(*argph, env);
        if (auto dot = cast<Dot_Expr>(arg)) {
            return make<Defined_Expression>(
                share(ph), dot->base_, dot->selector_);
        }
        if (auto slice = cast<Index_Expr>(arg)) {
            return make<Defined_Expression>(
                share(ph), slice->arg1_, Symbol_Expr(slice->arg2_));
        }
        throw Exception(At_Phrase(*argph, env),
            "defined: argument must be `expression.identifier`"
                " or `expression.[expression]`"
                " or `expression@expression`");
    }
    virtual void print_help(std::ostream& out) const override
    {
        out <<
            "defined (record . identifier)\n"
            "  True if a field named identifier is defined by record, otherwise false.\n"
            "  For example, given 'R={a:1}', then 'defined(R.a)' is true and 'defined(R.foo)' is false.\n"
            "\n"
            "defined (record .[ symbolExpr ])\n"
            "  Test the field named by the symbol after evaluating symbolExpr. If the field exists,\n"
            "  return true, otherwise false. This allows the field name to be computed at run time.\n"
            "  For example, 'defined(R.[#a])' is true and 'defined(R.[#foo])' is false.\n";
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

struct Builtin_Resolution : public Builtin
{
    virtual Shared<Meaning> to_meaning(const Identifier& id) const
    {
        return make<Constant>(share(id), Value{make<Uniform_Variable>(
            make_symbol("resolution"),
            std::string("u_resolution"),
            SC_Type::Array(SC_Type::Num(), 2),
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
    {make_symbol("Any"), make<Builtin_Value>(Value(make<Any_Type>()))},
    {make_symbol("Bool"), make<Builtin_Value>(Value(make<Bool_Type>()))},
    {make_symbol("Char"), make<Builtin_Value>(Value(make<Char_Type>()))},
    {make_symbol("Func"), make<Builtin_Value>(Value(make<Func_Type>()))},
    {make_symbol("Num"), make<Builtin_Value>(Value(make<Num_Type>()))},
    {make_symbol("Symbol"), make<Builtin_Value>(Value(make<Symbol_Type>()))},
    {make_symbol("Type"), make<Builtin_Value>(Value(make<Type_Type>()))},
    {make_symbol("time"), make<Builtin_Time>()},
    {make_symbol("resolution"), make<Builtin_Resolution>()},

    FUNCTION("as", F_as),
    FUNCTION("is", F_is),
    FUNCTION("Tuple", F_Tuple),
    FUNCTION("List", F_List),
    FUNCTION("Array", F_Array),
    FUNCTION("Struct", F_Struct),
    FUNCTION("Record", F_Record),
    FUNCTION("is_bool", F_is_bool),
    FUNCTION("is_char", F_is_char),
    FUNCTION("is_symbol", F_is_symbol),
    FUNCTION("is_num", F_is_num),
    FUNCTION("is_string", F_is_string),
    FUNCTION("is_list", F_is_list),
    FUNCTION("is_record", F_is_record),
    FUNCTION("is_primitive_func", F_is_primitive_func),
    FUNCTION("is_func", F_is_func),
    FUNCTION("bit", F_bit),
    FUNCTION("sqrt", F_sqrt),
    FUNCTION("log", F_log),
    FUNCTION("abs", F_abs),
    FUNCTION("floor", F_floor),
    FUNCTION("ceil", F_ceil),
    FUNCTION("trunc", F_trunc),
    FUNCTION("round", F_round),
    FUNCTION("frac", F_frac),
    FUNCTION("sign", F_sign),
    FUNCTION("sin", F_sin),
    FUNCTION("cos", F_cos),
    FUNCTION("tan", F_tan),
    FUNCTION("asin", F_asin),
    FUNCTION("acos", F_acos),
    FUNCTION("atan", F_atan),
    FUNCTION("phase", F_phase),
    FUNCTION("sinh", F_sinh),
    FUNCTION("cosh", F_cosh),
    FUNCTION("tanh", F_tanh),
    FUNCTION("asinh", F_asinh),
    FUNCTION("acosh", F_acosh),
    FUNCTION("atanh", F_atanh),
    FUNCTION("max", F_max),
    FUNCTION("min", F_min),
    FUNCTION("sum", F_sum),
    FUNCTION("not", F_not),
    FUNCTION("and", F_and),
    FUNCTION("or", F_or),
    FUNCTION("xor", F_xor),
    FUNCTION("lshift", F_lshift),
    FUNCTION("rshift", F_rshift),
    FUNCTION("bool32_sum", F_bool32_sum),
    FUNCTION("bool32_product", F_bool32_product),
    FUNCTION("bool32_to_nat", F_bool32_to_nat),
    FUNCTION("nat_to_bool32", F_nat_to_bool32),
    FUNCTION("bool32_to_float", F_bool32_to_float),
    FUNCTION("float_to_bool32", F_float_to_bool32),
    FUNCTION("select", F_select),
    FUNCTION("dot", F_dot),
    FUNCTION("mag", F_mag),
    FUNCTION("count", F_count),
    FUNCTION("fields", F_fields),
    FUNCTION("char", F_char),
    FUNCTION("ucode", F_ucode),
    FUNCTION("symbol", F_symbol),
    FUNCTION("string", F_string),
    FUNCTION("repr", F_repr),
    FUNCTION("match", F_match),
    FUNCTION("compose", F_compose),

    // top secret index API (aka lenses)
    {make_symbol("this"), make<Builtin_Value>(Value{make<This>()})},
    FUNCTION("tslice", F_tslice),
    FUNCTION("tpath", F_tpath),
    FUNCTION("amend", F_amend),

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
