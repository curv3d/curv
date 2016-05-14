/*
 * Copyright 2016 Doug Moen. See LICENCE.md file for terms of use.
 */
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
