// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_LOCATION_H
#define CURV_LOCATION_H

#include <curv/script.h>
#include <curv/token.h>
#include <aux/range.h>

namespace curv {

/// The place where an error occurred: the source file name,
/// and a character range within that source file, spanning either a
/// single token, or a parse tree node.
///
/// Using this interface, a CLI tool can report an error by printing
/// the filename and line/column numbers,
/// and an IDE can highlight the text range containing the error.
struct Location
{
    const Script& script_;
    Token token_;

    Location(const Script& script, Token token)
    : script_(script), token_(token)
    {}

    /// Modify location to start at 'tok'
    Location starting_at(Token tok) const;

    /// Modify location to end at 'tok'
    Location ending_at(Token tok) const;

    /// Name of script where error occurred.
    const std::string& scriptname() const { return script_.name; }

    /// Line number within script where error occurred.
    int lineno() const;

    /// Range of characters within script where error occurred.
    aux::Range<const char*> range() const;
};

} // namespace curv
#endif // header guard
