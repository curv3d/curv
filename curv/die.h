// Copyright Doug Moen 2016-2018.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_DIE_H
#define CURV_DIE_H

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
