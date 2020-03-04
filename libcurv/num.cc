// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/num.h>

#include <libcurv/exception.h>
#include <libcurv/reactive.h>
#include <libcurv/string.h>

#include <climits>
#include <cmath>

namespace curv {

bool is_num(Value a)
{
    if (a.is_num()) return true;
    auto r = a.maybe<Reactive_Value>();
    return r && r->sctype_ == SC_Type::Num();
}

int
num_to_int(double num, int lo, int hi, const Context& cx)
{
    double intf;
    double frac = modf(num, &intf);
    if (frac != 0.0)
        throw Exception(cx, stringify(num, " is not an integer"));
    if (intf < double(lo) || intf > double(hi))
        throw Exception(cx, stringify(
            intf, " is not in the range ",lo,"..",hi));
    return int(intf);
}

unsigned
num_to_nat(double num, const Context& cx)
{
    double intf;
    double frac = modf(num, &intf);
    if (frac != 0.0)
        throw Exception(cx, stringify(num, " is not an integer"));
    if (intf < 0.0 || intf > double(UINT_MAX))
        throw Exception(cx, stringify(
            intf, " is not in the range 0..", UINT_MAX));
    return unsigned(intf);
}

unsigned
bitcast_float_to_nat(double f)
{
    union { float f; unsigned n; } u;
    u.f = float(f);
    return u.n;
}

double
bitcast_nat_to_float(unsigned n)
{
    union { float f; unsigned n; } u;
    u.n = n;
    return double(u.f);
}

} // namespace curv
