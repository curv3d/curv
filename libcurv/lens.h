// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_LENS_H
#define LIBCURV_LENS_H

#include <libcurv/value.h>
#include <libcurv/context.h>

namespace curv {

Value lens_get(Value val, Value lens, const At_Syntax& cx);

} // namespace curv
#endif // header guard
