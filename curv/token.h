// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_TOKEN_H
#define CURV_TOKEN_H

#include <ostream>
#include <curv/script.h>

namespace curv {

/// \brief A token identified by the lexical analyzer.
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
    Token(const curv::Script& s) : script(&s) {}
    const Script* script;
    uint32_t white_first, first, last;
    enum Kind {
        k_ident,
        k_num,
        k_lparen,
        k_rparen,
        k_plus,
        k_minus,
        k_times,
        k_over,
        k_equate,
        k_end
    } kind;
    const std::string& scriptname() const { return script->name; }
    int lineno() const;

    const char* begin() const { return script->begin() + first; }
    const char* end() const { return script->begin() + last; }
    size_t size() const { return last - first; }
};

std::ostream& operator<<(std::ostream&, const Token&);

} // namespace curv
#endif // header guard
