// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/location.h>

using namespace curv;
using namespace aux;

auto Location::line_info() const
-> Line_Info
{
    // The line/col of a character position is ambiguous, if the position
    // points at the next position after a newline. Cases:
    // - The first index in a non-empty range is ambiguous:
    //   Use next line #, col 0.
    // - The last index in a non-empty range is ambiguous:
    //   Use prev line #, col == # chars in line including newline.
    // - Empty range, first==last, ambiguous (only happens for EOF token):
    //   Use prev line #, col == # chars in line including newline.
    Line_Info info;
    unsigned lineno = 0;
    unsigned colno = 0;
    unsigned linebegin = 0;
    uint32_t size = script_->size();
    for (uint32_t i = 0; i <= size; ++i) {
        if (i == token_.first) {
            info.start_line_num = lineno;
            info.start_column_num = colno;
            info.start_line_begin = linebegin;
        }
        if (i == token_.last) {
            info.end_line_num = lineno;
            info.end_column_num = colno;
        }
        if (i > 0 && (*script_)[i-1] == '\n') {
            // ambiguous character position, see above.
            ++lineno;
            linebegin = i;
            colno = 0;
            if (i == token_.first) {
                if (token_.first == token_.last) {
                    // a zero-length EOF token, preceded by \n.
                    // Moving the token to precede the \n so that the
                    // caret is positioned in a more readable place for write().
                    --info.start_column_num;
                    --info.end_column_num;
                } else {
                    // a non-empty token that starts at the beginning of a line
                    info.start_line_num = lineno;
                    info.start_column_num = colno;
                    info.start_line_begin = linebegin;
                }
            }
        }
        ++colno;
    }
    return info;
}

void
curv::Location::write(std::ostream& out) const
{
    // TODO: more expressive and helpful diagnostics.
    // Inspiration: http://clang.llvm.org/diagnostics.html
    // TODO: unicode underlining? a̲b̲c̲d̲e̲f̲
    if (!scriptname().empty())
        out << "file \"" << scriptname() << "\", ";
    auto info = line_info();
    if (info.start_line_num == info.end_line_num) {
        out << "line " << info.start_line_num+1 << "(";
        if (info.end_column_num - info.start_column_num <= 1)
            out << "column " << info.start_column_num+1;
        else
            out << "columns " << info.start_column_num+1
                << "-" << info.end_column_num;
        out << ")";
    } else {
        out << "lines "
            << info.start_line_num+1 << "(column " << info.start_column_num+1
            << ")-"
            << info.end_line_num+1 << "(column " << info.end_column_num << ")";
    }
    switch (token_.kind) {
    case Token::k_end:
        out << ", at end of script";
        break;
    default:
        break;
    }
    out << "\n  ";
    const char* line = script_->first + info.start_line_begin;
    unsigned len = 0;
    for (const char* p = line; p < script_->last && *p != '\n'; ++p) {
        if (*p == '\t')
            out << "  ";
        else
            out << *p;
        ++len;
    }
    out << "\n  ";
    struct Caret {
        bool first = true;
        void put(std::ostream& out, char c)
        {
            out << (first ? '^' : '-');
            if (c == '\t') out << '-';
            first = false;
        }
    } caret;
    unsigned startcol = info.start_column_num;
    unsigned endcol =
        info.end_line_num > info.start_line_num ? len : info.end_column_num;
    for (unsigned i = 0; i < len; ++i) {
        if (i >= startcol && i < endcol) {
            caret.put(out, line[i]);
        } else {
            if (line[i] == '\t') out << "  "; else out << ' ';
        }
    }
    if (startcol == len)
        out << '^';
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
