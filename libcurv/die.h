// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_DIE_H
#define LIBCURV_DIE_H

namespace curv {

#define XSTRINGIZE(s) STRINGIZE(s)
#define STRINGIZE(s) #s
/*
#define foo 4
STRINGIZE (foo)
     → "foo"
XSTRINGIZE (foo)
     → XSTRINGIZE (4)
     → STRINGIZE (4)
     → "4"
*/

#define die(msg) die_impl(__FILE__, XSTRINGIZE(__LINE__), msg)
[[noreturn]] void die_impl(const char*, const char*, const char*);

} // namespace curv
#endif // header guard
