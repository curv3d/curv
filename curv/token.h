// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <cstdint>

#ifndef CURV_TOKEN_H
#define CURV_TOKEN_H

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
/// as the range (first_white, first). This is needed for processing
/// thingiverse customizer attributes, which are hidden in comments.
/// The trailing whitespace at the end of the script is attached to the zero
/// length k_end token identified at the end of script.
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
    uint32_t first_white, first, last;
    enum Kind {
        k_missing,    ///! not a token; marks an uninitialized token variable.
        k_phrase,     ///! text spanned by a parse tree node: 2 or more tokens
        k_bad_token,  ///! a malformed token
        k_bad_utf8,   ///! a malformed UTF-8 sequence
        k_ident,
        k_if,
        k_else,
        k_let,
        k_num,        ///! floating point numeral
        k_string,
        k_lparen,
        k_rparen,
        k_lbracket,
        k_rbracket,
        k_lbrace,
        k_rbrace,
        k_dot,
        k_comma,
        k_semicolon,
        k_at,               ///! `@` operator
        k_right_arrow,      ///! `->` operator
        k_power,            ///! `^` operator
        k_plus,
        k_minus,
        k_times,
        k_over,             ///! `/` operator
        k_equal,            ///! `==` operator
        k_not_equal,        ///! `!=` operator
        k_less,             ///! `<` operator
        k_less_or_equal,    ///! `<=` operator
        k_greater,          ///! `>` operator
        k_greater_or_equal, ///! `>=` operator
        k_not,              ///! `!` operator
        k_equate,           ///! `=` operator
        k_and,              ///! `&&` operator
        k_or,               ///! `||` operator
        k_end               ///! end of script
    } kind;

    Token() : kind(k_missing) {}
};

} // namespace curv
#endif // header guard
