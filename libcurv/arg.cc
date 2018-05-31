// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <cmath>
#include <libcurv/arg.h>
#include <libcurv/exception.h>
#include <libcurv/phrase.h>

namespace curv {

// TODO: Most of the following functions are redundant with the Value API.

bool arg_to_bool(Value val, const Context& ctx)
{
    if (!val.is_bool())
        throw Exception(ctx, "not boolean");
    return val.get_bool_unsafe();
}

auto arg_to_list(Value val, const Context& ctx)
-> List&
{
    if (!val.is_ref())
        throw Exception(ctx, "not a list");
    Ref_Value& ref( val.get_ref_unsafe() );
    if (ref.type_ != Ref_Value::ty_list)
        throw Exception(ctx, "not a list");
    return (List&)ref;
}

int arg_to_int(Value val, int lo, int hi, const Context& ctx)
{
    if (!val.is_num())
        throw Exception(ctx, "not an integer");
    double num = val.get_num_unsafe();
    double intf;
    double frac = modf(num, &intf);
    if (frac != 0.0)
        throw Exception(ctx, stringify(num," is not an integer"));
    if (intf < (double)lo || intf > (double)hi)
        throw Exception(ctx, stringify(intf," is not in range ",lo,"..",hi));
    return (int)intf;
}

auto arg_to_num(Value val, const Context& ctx)
-> double
{
    if (!val.is_num())
        throw Exception(ctx, "not a number");
    return val.get_num_unsafe();
}

} // namespace curv
