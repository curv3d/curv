// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/func_loc.h>

#include <libcurv/ansi_colour.h>
#include <libcurv/format.h>
#include <libcurv/function.h>
#include <libcurv/json.h>

namespace curv {

Src_Loc::Line_Info
Src_Loc::line_info() const
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
        if (i > 0 && (*source_)[i-1] == '\n') {
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

const char*
putsrcline(
    std::ostream& out,
    unsigned ln, unsigned lnwidth,
    const char* line, const char* end,
    bool underlined, unsigned startcol, unsigned endcol,
    bool colour)
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
    if (colour) out << AC_LINENO;
    out << lnbuf << "|";
    if (colour) out << AC_RESET;

    // Underlining is forced if there is a single source line. Otherwise,
    // is the entire line part of the selected text? Then we output '>'.
    // Otherwise, we output ' ' and output underlining on the next line.
    if (!underlined) {
        // Are there nonwhite characters outside the selection region?
        if (startcol > firstnw-line || endcol < lastnw-line)
            underlined = true;
    }
    if (underlined)
        out << ' ';
    else {
        if (colour) out << AC_CARET;
        out << '>';
        if (colour) out << AC_RESET;
    }

    // Output the source line.
    for (const char* p = line; p < endline; ++p) {
        if (*p == '\t')
            out << "  ";
        else
            out << *p;
    }

    // Output the underlining.
    if (underlined) {
        if (startcol == endcol) ++endcol;
        out << "\n";
        for (unsigned i = lnwidth + 2; i > 0; --i)
            out << ' ';
        unsigned linelen = endline - line;
        if (colour) out << AC_CARET;
        for (unsigned i = 0; i < linelen; ++i) {
            if (i >= startcol && i < endcol) {
                if (line[i] == '\t') out << "^^"; else out << '^';
            } else {
                if (line[i] == '\t') out << "  "; else out << ' ';
            }
        }
        if (startcol == linelen)
            out << '^';
        if (colour) out << AC_RESET;
    }

    // Return start of next line.
    if (endline < end)
        return endline + 1;
    else
        return nullptr;
}

void
Src_Loc::write_code(std::ostream& out, bool colour) const
{
    auto info = line_info();

    // Output underlined program text. No final newline.
    // Inspired by http://elm-lang.org/blog/compiler-errors-for-humans
    if (info.start_line_num == info.end_line_num) {
        putsrcline(out,
            info.start_line_num, ndigits(info.start_line_num+1),
            source_->first + info.start_line_begin, source_->last,
            true, info.start_column_num, info.end_column_num,
            colour);
    } else {
        unsigned lnwidth =
            std::max(ndigits(info.start_line_num+1),ndigits(info.end_line_num+1));
        unsigned ln = info.start_line_num;
        const char* p = source_->first + info.start_line_begin;
        for (;;) { // each line
            if (ln > info.start_line_num) out << '\n';
            p = putsrcline(out,
                ln, lnwidth,
                p, source_->last,
                false,
                ln == info.start_line_num ? info.start_column_num : 0,
                ln == info.end_line_num ? info.end_column_num : unsigned(~0),
                colour);
            if (ln == info.end_line_num)
                break;
            ++ln;
        }
    }
}

void
Func_Loc::write(std::ostream& out, bool colour, bool many) const
{
    // TODO: more expressive and helpful diagnostics.
    // Inspiration: http://clang.llvm.org/diagnostics.html
    // and http://elm-lang.org/blog/compiler-errors-for-humans

    if (srcloc_.source().no_contents()) {
        out << "at file \"" << srcloc_.filename() << "\"";
        return;
    }

    // Output filename and function name, followed by newline.
    if (many || !srcloc_.filename().empty() || (func_ && func_->name_)) {
        out << "at";
        if (!srcloc_.filename().empty())
            out << " file \"" << srcloc_.filename() << "\"";
        if (func_ && func_->name_) {
            out << " function " << func_->name_;
            for (unsigned i = 0; i < func_->argpos_; ++i)
                out << " _";
        }
        out << ":\n";
    }

    // Output underlined program text. No final newline.
    srcloc_.write_code(out, colour);
}

void
Func_Loc::write_json(std::ostream& out) const
{
    if (srcloc_.source().no_contents()) {
        out << "{";
        out << "\"filename\":";
        write_json_string(srcloc_.filename().c_str(), out);
        out << "}";
        return;
    }

    auto info = srcloc_.line_info();

    out << "{\"start\":{\"char\":" << srcloc_.token().first_
        << ",\"line_begin\":" << info.start_line_begin
        << ",\"line\":" << info.start_line_num
        << ",\"column\":" << info.start_column_num
        << "},\"end\":{\"char\":" << srcloc_.token().last_
        << ",\"line\":" << info.end_line_num
        << ",\"column\":" << info.end_column_num
        << "}";
    if (!srcloc_.filename().empty()) {
        out << ",\"filename\":";
        write_json_string(srcloc_.filename().c_str(), out);
    }
    out << "}";
}

Range<const char*>
Src_Loc::range() const
{
    return Range<const char*>(
        source_->first + token_.first_, source_->first + token_.last_);
}

Src_Loc
Src_Loc::starting_at(Token tok) const
{
    Src_Loc loc = *this;
    if (tok.kind_ != Token::k_missing) {
        loc.token_.first_white_ = tok.first_white_;
        loc.token_.first_ = tok.first_;
        loc.token_.kind_ = Token::k_phrase;
    }
    return loc;
}

Src_Loc
Src_Loc::ending_at(Token tok) const
{
    Src_Loc loc = *this;
    if (tok.kind_ != Token::k_missing) {
        loc.token_.last_ = tok.last_;
        loc.token_.kind_ = Token::k_phrase;
    }
    return loc;
}

} // namespace curv
