// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/token.h>

using namespace curv;

void
curv::Token::write(std::ostream& out, const Script& script)
const
{
    if (kind == Token::k_end)
        out << "<end-of-script>";
    else {
        for (uint32_t i = first; i < last; ++i)
            out << script.begin()[i];
    }
}
