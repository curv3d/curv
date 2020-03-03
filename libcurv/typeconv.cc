// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/typeconv.h>

#include <libcurv/exception.h>
#include <libcurv/string.h>

#include <climits>
#include <cmath>

namespace curv {

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

Shared<const List>
nat_to_bool32(unsigned n)
{
    Shared<List> result = List::make(32);
    for (unsigned i = 0; i < 32; ++i)
        result->at(i) = {(n & (1 << i)) != 0};
    return result;
}

unsigned
bool32_to_nat(Shared<const List> li, const Context& cx)
{
    unsigned out = 0;
    for (unsigned i = 0; i < 32; ++i) {
        out |= unsigned(li->at(i).to_bool(cx)) << i;
    }
    return out;
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
