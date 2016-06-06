// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/location.h>

using namespace curv;
using namespace aux;

int
curv::Location::lineno() const
{
    int lineno = 1;
    for (uint32_t i = 0; i < token_.first; ++i) {
        if (script_->first[i] == '\n')
            ++lineno;
    }
    return lineno;
}

Range<const char*>
curv::Location::range() const
{
    return Range<const char*>(
        script_->first + token_.first, script_->first + token_.last);
}

Location
curv::Location::starting_at(Token tok) const
{
    Location loc = *this;
    if (tok.kind != Token::k_missing) {
        loc.token_.first_white = tok.first_white;
        loc.token_.first = tok.first;
        loc.token_.kind = Token::k_phrase;
    }
    return loc;
}

Location
curv::Location::ending_at(Token tok) const
{
    Location loc = *this;
    if (tok.kind != Token::k_missing) {
        loc.token_.last = tok.last;
        loc.token_.kind = Token::k_phrase;
    }
    return loc;
}

void
curv::Location::write(std::ostream& out) const
{
    out << "file " << scriptname()
        << ", line " << lineno();
    switch (token_.kind) {
    case Token::k_end:
        out << ", at end of script";
        return;
    default:
        out << ", token " << range();
    }
}
