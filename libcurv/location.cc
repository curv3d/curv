// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/location.h>

namespace curv {

Location::Line_Info
Location::line_info() const
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
    for (uint32_t i = 0; i <= token_.last_; ++i) {
        if (i == token_.first_) {
            info.start_line_num = lineno;
            info.start_column_num = colno;
            info.start_line_begin = linebegin;
        }
        if (i == token_.last_) {
            info.end_line_num = lineno;
            info.end_column_num = colno;
        }
        if (i > 0 && (*script_)[i-1] == '\n') {
            // ambiguous character position, see above.
            ++lineno;
            linebegin = i;
            colno = 0;
            if (i == token_.first_) {
                if (token_.first_ == token_.last_) {
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

// # of decimal digits in n
unsigned
ndigits(unsigned n)
{
    unsigned count = 1;
    for (;;) {
        if (n < 10)
            return count;
        n = n / 10;
        ++count;
    }
}

//
const char*
putsrcline(
    std::ostream& out,
    unsigned ln, unsigned lnwidth,
    const char* line, const char* end,
    char mode, unsigned startcol, unsigned endcol)
{
    // Find end of line, first and last non-whitespace.
    const char* endline = (const char*) memchr(line, '\n', end - line);
    if (endline == nullptr) endline = end;
    const char* firstnw = line;
    while (firstnw < endline && isspace(firstnw[0]))
        ++firstnw;
    const char* lastnw = endline;
    while (lastnw > line && isspace(lastnw[-1]))
        --lastnw;

    // Put line number.
    char lnbuf[16];
    snprintf(lnbuf, 16, "%*u", lnwidth, ln+1);
    out << lnbuf << "|";

    // Is the entire line part of the selected text? Then we output '>'.
    // Otherwise, we output ' ' and output underlining on the next line.
    bool underlined;
    char caret;
    if (mode == '1') {
        caret = '^';
        underlined = true;
    } else {
        // Are there nonwhite characters outside the selection region?
        if (startcol > firstnw-line || endcol < lastnw-line) {
            underlined = true;
            caret = mode;
        } else
            underlined = false;
    }
    out << (underlined ? ' ' : '>');

    // Output the source line.
    for (const char* p = line; p < endline; ++p) {
        if (*p == '\t')
            out << "  ";
        else
            out << *p;
    }

    // Output the underlining.
    if (underlined) {
        out << "\n";
        for (unsigned i = lnwidth + 2; i > 0; --i)
            out << ' ';
        struct Underline {
            bool first = true;
            char caret = '^';
            char dash = '^'; // was '-'
            void put(std::ostream& out, char c)
            {
                out << (first ? caret : dash);
                if (c == '\t') out << dash;
                first = false;
            }
        } under;
        (void)caret; //was: under.caret = caret;
        unsigned linelen = endline - line;
        for (unsigned i = 0; i < linelen; ++i) {
            if (i >= startcol && i < endcol) {
                under.put(out, line[i]);
            } else {
                if (line[i] == '\t') out << "  "; else out << ' ';
            }
        }
        if (startcol == linelen)
            out << '^';
    }

    // Return start of next line.
    if (endline < end)
        return endline + 1;
    else
        return nullptr;
}

void
Location::write(std::ostream& out) const
{
    // TODO: more expressive and helpful diagnostics.
    // Inspiration: http://clang.llvm.org/diagnostics.html
    // and http://elm-lang.org/blog/compiler-errors-for-humans
    // TODO: unicode underlining? a̲b̲c̲d̲e̲f̲
    // TODO: mark bad text in red, ANSI escapes for console output?

    auto info = line_info();

    // Output filename and line number, followed by newline.
    out << "at";
    if (!scriptname().empty())
        out << " file \"" << scriptname() << "\"";
    out << ":\n";
#if 0
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
    switch (token_.kind_) {
    case Token::k_end:
        out << ", at end of script";
        break;
    default:
        break;
    }
#endif

    // Output underlined program text. No final newline.
    // Inspired by http://elm-lang.org/blog/compiler-errors-for-humans
    if (info.start_line_num == info.end_line_num) {
        putsrcline(out,
            info.start_line_num, ndigits(info.start_line_num+1),
            script_->first + info.start_line_begin, script_->last,
            '1', info.start_column_num, info.end_column_num);
    } else {
        unsigned lnwidth =
            std::max(ndigits(info.start_line_num+1),ndigits(info.end_line_num+1));
        unsigned ln = info.start_line_num;
        const char* p = script_->first + info.start_line_begin;
        for (;;) { // each line
            if (ln > info.start_line_num) out << '\n';
            p = putsrcline(out,
                ln, lnwidth,
                p, script_->last,
                ln == info.start_line_num ? '^' : '-',
                ln == info.start_line_num ? info.start_column_num : 0,
                ln == info.end_line_num ? info.end_column_num : unsigned(~0));
            if (ln == info.end_line_num)
                break;
            ++ln;
        }
    }
/*
const char*
putsrcline(
    unsigned ln, unsigned lnwidth,
    const char* line, const char* end,
    char mode, unsigned startcol, unsigned endcol)
*/
#if 0
    if (info.start_line_num == info.end_line_num) {
        out << info.start_line_num+1 << "| ";
        const char* line = script_->first + info.start_line_begin;
        unsigned len = 0;
        for (const char* p = line; p < script_->last && *p != '\n'; ++p) {
            if (*p == '\t')
                out << "  ";
            else
                out << *p;
            ++len;
        }
        out << "\n";
        for (unsigned i = ndigits(info.start_line_num+1) + 2; i > 0; --i)
            out << ' ';
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
        if (startcol == endcol)
            ++endcol;
        for (unsigned i = 0; i < len; ++i) {
            if (i >= startcol && i < endcol) {
                caret.put(out, line[i]);
            } else {
                if (line[i] == '\t') out << "  "; else out << ' ';
            }
        }
        if (startcol == len)
            out << '^';
    } else {
        unsigned lnwidth =
            std::max(ndigits(info.start_line_num+1),ndigits(info.end_line_num+1));
        unsigned ln = info.start_line_num;
        const char* p = script_->first + info.start_line_begin;
        for (;;) { // each line
            char buf[16];
            snprintf(buf, 16, "%*u", lnwidth, ln+1);
            out << buf << "|>";
            while (p < script_->last && *p != '\n')
                out << *p++;
            if (ln == info.end_line_num)
                break;
            out << *p++;
            ++ln;
        }
    }
#endif
}

Range<const char*>
Location::range() const
{
    return Range<const char*>(
        script_->first + token_.first_, script_->first + token_.last_);
}

Location
Location::starting_at(Token tok) const
{
    Location loc = *this;
    if (tok.kind_ != Token::k_missing) {
        loc.token_.first_white_ = tok.first_white_;
        loc.token_.first_ = tok.first_;
        loc.token_.kind_ = Token::k_phrase;
    }
    return loc;
}

Location
Location::ending_at(Token tok) const
{
    Location loc = *this;
    if (tok.kind_ != Token::k_missing) {
        loc.token_.last_ = tok.last_;
        loc.token_.kind_ = Token::k_phrase;
    }
    return loc;
}

} // namespace curv
