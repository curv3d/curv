// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_TOKEN_H
#define CURV_TOKEN_H

#include <ostream>
#include <curv/script.h>
#include <aux/range.h>

namespace curv {

/// A lexeme identified by the lexical analyzer,
/// or the text spanned by a parse tree node.
///
/// A token is a contiguous substring of a script,
/// represented as the half-open range (first,last).
/// We use 32 bit indexes into the script, rather than 64 bit pointers,
/// to save space.
///
/// Each token remembers the preceding whitespace and comments
/// as the range (white_first, first). This is needed for processing
/// thingiverse customizer attributes, which are hidden in comments.
/// The trailing whitespace at the end of the script is attached to the zero
/// length k_end token identified at the end of script.
///
/// Each token remembers what script it came from. This is needed for
/// reporting errors in the presence of OpenSCAD `include` processing,
/// which jumbles multiple scripts into a single token stream.
///
/// We don't copy string data out of the script:
/// this is a zero copy lexical analyzer.
///
/// We don't store line numbers or column numbers. Those can be reconstructed
/// by scanning the script. This scanning is expensive, but the cost is only
/// paid when there is an error to report, and if there is no error, we save
/// time and memory.
///
/// The lexical analyzer doesn't convert tokens into their semantic
/// representation. Eg, we don't convert numerals into floating point values.
/// That's done at a higher level. This simplifies the representation of tokens.
struct Token
{
    uint32_t white_first, first, last;
    enum Kind {
        k_phrase,     ///! text spanned by a parse tree node: 1 or more tokens
        k_bad_token,  ///! a malformed token
        k_bad_utf8,   ///! a malformed UTF-8 sequence
        k_ident,
        k_num,        ///! floating point numeral
        k_lparen,
        k_rparen,
        k_plus,
        k_minus,
        k_times,
        k_over,       ///! `/` operator
        k_equate,     ///! `=` operator
        k_end         ///! end of script
    } kind;

    inline aux::Range<const char*> range(const curv::Script& scr) const
    {
        return aux::Range<const char*>(scr.begin() + first, scr.begin() + last);
    }

    void write(std::ostream&, const Script&) const;
};

} // namespace curv
#endif // header guard
