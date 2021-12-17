// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_FAIL_H
#define LIBCURV_FAIL_H

namespace curv {

enum class Fail { soft, hard };
#define FAIL(fl, result, cx, msg) \
    if (fl==Fail::soft) return result; else throw Exception(cx,msg)
#define TRY_DEF(var, expr) \
    auto var = expr; if (!var) return {}
#define UNWRAP_DEF(var, optexpr) \
    auto optional_##var = optexpr; if (!optional_##var) return {}; \
    auto var = *optional_##var

} // namespace curv
#endif // header guard
