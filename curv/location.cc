// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/location.h>

using namespace curv;
using namespace aux;

/*
    struct Line_Info
    {
        unsigned start_line_num;
        unsigned start_column_num;
        unsigned end_line_num;
        unsigned end_column_num;
        aux::Range<const char*> start_line_text;
    };
*/

auto Location::line_info() const
-> Line_Info
{
    Line_Info info;
    unsigned lineno = 0;
    unsigned colno = 0;
    uint32_t i = 0;
    for (; i < token_.first; ++i) {
        if (script_->first[i] == '\n') {
            ++lineno;
            colno = 0;
        } else
            ++colno;
    }
    info.start_line_num = lineno;
    info.start_column_num = colno;
    for (; i < token_.last; ++i) {
        if (script_->first[i] == '\n') {
            ++lineno;
            colno = 0;
        } else
            ++colno;
    }
    info.end_line_num = lineno;
    info.end_column_num = colno;
    return info;
}

void
curv::Location::write(std::ostream& out) const
{
    if (!scriptname().empty())
        out << "file " << scriptname() << ", ";
    auto info = line_info();
    if (info.start_line_num == info.end_line_num) {
        out << "line " << info.start_line_num+1 << "[";
        if (info.end_column_num - info.start_column_num <= 1)
            out << info.start_column_num+1;
        else
            out << info.start_column_num+1 << "-" << info.end_column_num;
        out << "]";
    } else {
        out << "lines "
            << info.start_line_num+1 << "[" << info.start_column_num+1 << "]-"
            << info.end_line_num+1 << "[" << info.end_column_num << "]";
    }
    switch (token_.kind) {
    case Token::k_end:
        out << ", at end of script";
        return;
    default:
        out << ", token " << range();
    }
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
