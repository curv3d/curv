// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <cstdint>

#ifndef LIBCURV_TOKEN_H
#define LIBCURV_TOKEN_H

namespace curv {

/// A lexeme identified by the lexical analyser,
/// or the text spanned by a parse tree node.
///
/// A token is a contiguous substring of a source,
/// represented as the half-open range (first,last).
/// We use 32 bit indexes into the source, rather than 64 bit pointers,
/// to save space.
///
/// Each token remembers the preceding whitespace and comments
/// as the range (first_white, first). This is needed for processing
/// thingiverse customizer attributes, which are hidden in comments.
/// The trailing whitespace at the end of the source is attached to the zero
/// length k_end token identified at the end of source.
///
/// We don't copy string data out of the source:
/// this is a zero copy lexical analyser.
///
/// We don't store line numbers or column numbers. Those can be reconstructed
/// by scanning the source. This scanning is expensive, but the cost is only
/// paid when there is an error to report, and if there is no error, we save
/// time and memory.
///
/// The lexical analyser doesn't convert tokens into their semantic
/// representation. Eg, we don't convert numerals into floating point values.
/// That's done at a higher level. This simplifies the representation of tokens.
struct Token
{
    Token() {}

    // This constructor creates a simple 'generic' token for error messages,
    // not suitable for input to the parser.
    Token(uint32_t first, uint32_t last)
    :
        first_white_(first),
        first_(first),
        last_(last),
        kind_(k_phrase)
    {}

    uint32_t first_white_ = 0, first_ = 0, last_ = 0;
    enum Kind {
        k_missing,    ///! not a token; marks an uninitialized token variable.
        k_phrase,     ///! text spanned by a parse tree node: 2 or more tokens
        k_bad_token,  ///! a malformed token
        k_bad_utf8,   ///! a malformed UTF-8 sequence or unsupported code point
        k_ident,
            k_by,
            k_do,
            k_else,
            k_for,
            k_if,
            k_in,
            k_include,
            k_let,
            k_local,
            k_parametric,
            k_var,
            k_where,
            k_while,
        k_symbol,           ///! symbol literal, such as #foo
        k_num,              ///! floating point numeral
        k_hexnum,           ///! hexadecimal numeral
        k_quote,            ///! `"`
        k_string_segment,   ///! sequence of unescaped chars in a string literal
        k_string_newline,   ///! newline + next line indent, in string literal
        k_char_escape,      ///! escaped character sequence in a string literal
        k_dollar_paren,     ///! $( in a string literal
        k_dollar_brace,     ///! ${ in a string literal
        k_dollar_bracket,   ///! $[ in a string literal
        k_dollar_ident,     ///! $identifier in a string literal
        k_lparen,
        k_rparen,
        k_lbracket,
        k_rbracket,
        k_lbrace,
        k_rbrace,
        k_dot,
        k_comma,
        k_semicolon,
        k_colon,
        k_backtick,         ///! `
        k_at,               ///! `@` operator
        k_right_arrow,      ///! `->` operator
        k_power,            ///! `^` operator
        k_plus,
        k_minus,
        k_times,
        k_over,             ///! `/` operator
        k_range,            ///! `..` operator
        k_open_range,       ///! `..<` operator
        k_ellipsis,         ///! `...` operator
        k_equal,            ///! `==` operator
        k_not_equal,        ///! `!=` operator
        k_less,             ///! `<` operator
        k_less_or_equal,    ///! `<=` operator
        k_greater,          ///! `>` operator
        k_greater_or_equal, ///! `>=` operator
        k_not,              ///! `!` operator
        k_equate,           ///! `=` operator
        k_assign,           ///! `:=` operator
        k_colon_colon,      ///! `::` operator
        k_and,              ///! `&&` operator
        k_or,               ///! `||` operator
        k_left_call,        ///! `<<` operator
        k_right_call,       ///! `>>` operator
        k_end               ///! end of source
    } kind_ = k_missing;
};

} // namespace curv
#endif // header guard
