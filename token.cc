// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/token.h>

using namespace curv;

int
curv::Token::lineno() const
{
    int lineno = 1;
    for (uint32_t i = 0; i < first; ++i) {
        if (script->first[i] == '\n')
            ++lineno;
    }
    return lineno;
}

std::ostream&
curv::operator<<(std::ostream& out, const Token& tok)
{
    if (tok.kind == Token::k_end)
        out << "<end-of-script>";
    else {
        for (uint32_t i = tok.first; i < tok.last; ++i)
            out << tok.script->begin()[i];
    }
    return out;
}
