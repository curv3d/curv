// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_TYPECONV_H
#define LIBCURV_TYPECONV_H

#include <libcurv/list.h>

namespace curv {

struct Context;

int num_to_int(double n, int lo, int hi, const Context&);
unsigned num_to_nat(double n, const Context&);
Shared<const List> nat_to_bool32(unsigned);
unsigned bool32_to_nat(Shared<const List>, const Context&);

} // namespace curv
#endif // header guard
