// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_FAIL_H
#define LIBCURV_FAIL_H

#include <libcurv/exception.h>

namespace curv {

enum class Fail { soft, hard };
#define FAIL(fl, result, cx, msg) \
    if (fl==Fail::soft) return result; else throw Exception(cx,msg)

} // namespace curv
#endif // header guard
