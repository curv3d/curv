// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_NUM_H
#define LIBCURV_NUM_H

#include <libcurv/value.h>

namespace curv {

struct Context;

bool is_num(Value a);
int num_to_int(double n, int lo, int hi, const Context&);
bool num_is_int(double);
unsigned num_to_nat(double n, const Context&);
unsigned bitcast_float_to_nat(double);
double bitcast_nat_to_float(unsigned);

} // namespace curv
#endif // header guard
