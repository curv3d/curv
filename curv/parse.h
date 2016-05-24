// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_PARSE_H
#define CURV_PARSE_H

#include <curv/syntax.h>
#include <curv/script.h>

namespace curv {

aux::Shared_Ptr<Syntax> parse(const Script&);

} // namespace curv
#endif // header guard
