// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_BOOL_H
#define LIBCURV_BOOL_H

#include <libcurv/list.h>

namespace curv {

struct Context;

bool is_bool(Value a);
void assert_bool(Value a, const Context&);
Shared<const List> nat_to_bool32(unsigned);
unsigned bool32_to_nat(Shared<const List>, const Context&);

} // namespace curv
#endif // header guard
