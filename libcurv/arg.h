// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_ARG_H
#define LIBCURV_ARG_H

#include <libcurv/list.h>
#include <libcurv/context.h>

namespace curv {

List& arg_to_list(Value, const Context&);
int arg_to_int(Value, int, int, const Context&);

} // namespace curv
#endif // header guard
