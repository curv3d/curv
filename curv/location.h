// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_LOCATION_H
#define CURV_LOCATION_H

#include <curv/script.h>
#include <curv/token.h>
#include <aux/range.h>
#include <curv/shared.h>
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
    Shared<const Script> script_;
    Token token_;

public:
    Location(const Script& script, Token token)
    :
        script_(share(script)),
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
    const String& scriptname() const { return *script_->name; }

    /// Range of characters within script where error occurred.
    aux::Range<const char*> range() const;

    /// output the location part of an exception message (no final newline)
    void write(std::ostream&) const;

    /// Line and column information for a Location
    ///
    /// Line and column numbers begin at 0.
    /// [start_column_num,end_column_num) is a half-open range.
    /// start_line_begin is the 0 based byte index into the script of the
    /// start line.
    struct Line_Info
    {
        unsigned start_line_num;
        unsigned start_column_num;
        unsigned end_line_num;
        unsigned end_column_num;
        unsigned start_line_begin;
    };
    Line_Info line_info() const;
};

} // namespace curv
#endif // header guard
