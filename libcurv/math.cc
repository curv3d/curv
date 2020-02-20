// Copyright 2017-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/math.h>

#include <libcurv/context.h>
#include <libcurv/meaning.h>
#include <libcurv/prim.h>
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

bool is_list(Value a)
{
    if (a.dycast<List>())
        return true;
    if (auto r = a.dycast<Reactive_Value>())
        return r->sctype_.is_list();
    return false;
}
size_t list_count(Value val)
{
    if (val.is_ref()) {
        auto& ref = val.to_ref_unsafe();
        switch (ref.type_) {
        case Ref_Value::ty_list:
          {
            auto list = (List*)&ref;
            return list->size();
          }
        case Ref_Value::ty_reactive:
          {
            auto rx = (Reactive_Value*)&ref;
            if (rx->sctype_.is_list())
                return rx->sctype_.count();
          }
        }
    }
    return 1;
}
Value list_elem(Value val, size_t i, const At_Syntax& cx)
{
    if (val.is_ref()) {
        auto& ref = val.to_ref_unsafe();
        switch (ref.type_) {
        case Ref_Value::ty_list:
          {
            auto list = (List*)&ref;
            return list->at(i);
          }
        case Ref_Value::ty_reactive:
          {
            auto rx = (Reactive_Value*)&ref;
            auto ph = share(cx.syntax());
            Shared<List_Expr> index = List_Expr::make(
                {make<Constant>(ph, Value(double(i)))},
                ph);
            index->init();
            return {make<Reactive_Expression>(
                rx->sctype_.abase(),
                make<Call_Expr>(ph, rx->expr(), index),
                cx)};
          }
        }
    }
    return {};
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
            result = Add_Op::call(cx, result,
                Multiply_Op::call(cx, av->at(i), bv->at(i)));
        return result;
    }
}

} // namespace curv
