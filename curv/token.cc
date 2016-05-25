// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/token.h>

using namespace curv;

int
curv::Token::lineno(const Script& script) const
{
    int lineno = 1;
    for (uint32_t i = 0; i < first; ++i) {
        if (script.first[i] == '\n')
            ++lineno;
    }
    return lineno;
}

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
