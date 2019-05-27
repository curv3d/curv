// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_LOCATION_H
#define LIBCURV_LOCATION_H

#include <libcurv/source.h>
#include <libcurv/token.h>
#include <libcurv/range.h>
#include <libcurv/shared.h>
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
    Shared<const Source> source_;
    Token token_;

public:
    Location() {}

    Location(Shared<const Source> source, Token token)
    :
        source_(std::move(source)),
        token_(std::move(token))
    {}

    /// Modify location to start at 'tok'
    Location starting_at(Token tok) const;

    /// Modify location to end at 'tok'
    Location ending_at(Token tok) const;

    /// Source where error occurred.
    const Source& source() const { return *source_; }

    /// Index of text range where error occurred.
    Token token() const { return token_; }

    /// Name of source file where error occurred.
    const String& filename() const { return *source_->name_; }

    /// Range of characters within source where error occurred.
    Range<const char*> range() const;

    /// Output the location part of an exception message (no final newline).
    /// The `colour` flag enables colour text using ANSI ESC sequences.
    void write(std::ostream&, bool colour, bool many) const;

    void write_json(std::ostream&) const;

    /// Line and column information for a Location
    ///
    /// Line and column numbers begin at 0.
    /// [start_column_num,end_column_num) is a half-open range.
    /// start_line_begin is the 0 based byte index into the source of the
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
