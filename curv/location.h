// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_LOCATION_H
#define CURV_LOCATION_H

#include <curv/script.h>
#include <curv/token.h>
#include <aux/range.h>
#include <aux/shared.h>
#include <ostream>

namespace curv {

/// The place where an error occurred: the source file name,
/// and a character range within that source file, spanning either a
/// single token, or a parse tree node.
///
/// Using this interface, a CLI tool can report an error by printing
/// the filename, line/column numbers, the line of text containing the
/// error with a ^ under the first character of the text range,
/// and an IDE can highlight the text range containing the error.
struct Location
{
private:
    aux::Shared_Ptr<const Script> script_;
    Token token_;

public:
    Location(const Script& script, Token token)
    :
        script_(aux::Shared_Ptr<const Script>(&script)),
        token_(std::move(token))
    {}

    /// Modify location to start at 'tok'
    Location starting_at(Token tok) const;

    /// Modify location to end at 'tok'
    Location ending_at(Token tok) const;

    /// Script where error occurred.
    const Script& script() const { return *script_; }

    /// Index of text range where error occurred.
    Token token() const { return token_; }

    /// Name of script where error occurred.
    const std::string& scriptname() const { return script_->name; }

    /// Line number within script where error occurred.
    int lineno() const;

    /// Range of characters within script where error occurred.
    aux::Range<const char*> range() const;

    /// output the location part of an exception message (no final newline)
    void write(std::ostream&) const;
};

} // namespace curv
#endif // header guard
