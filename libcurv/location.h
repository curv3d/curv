// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_LOCATION_H
#define LIBCURV_LOCATION_H

#include <libcurv/fname.h>
#include <libcurv/range.h>
#include <libcurv/source.h>
#include <libcurv/token.h>
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
struct Src_Loc
{
private:
    Shared<const Source> source_;
    Token token_;

public:
    Src_Loc() {}

    Src_Loc(Shared<const Source> source, Token token)
    :
        source_(std::move(source)),
        token_(std::move(token))
    {}

    /// Modify location to start at 'tok'
    Src_Loc starting_at(Token tok) const;

    /// Modify location to end at 'tok'
    Src_Loc ending_at(Token tok) const;

    /// Source where error occurred.
    const Source& source() const { return *source_; }

    /// Index of text range where error occurred.
    Token token() const { return token_; }

    /// Name of source file where error occurred.
    const String& filename() const { return *source_->name_; }

    /// Range of characters within source where error occurred.
    Range<const char*> range() const;

    /// Output the lines of source code containing the location,
    /// with underlining.
    void write_code(std::ostream&, bool colour) const;

    /// Line and column information for a Src_Loc
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

// A Func_Loc is location data used to print an element of a stack trace.
struct Func_Loc
{
    Func_Loc(Src_Loc srcloc)
    :
        srcloc_(srcloc)
    {}
    Func_Loc(FName fn, Src_Loc srcloc)
    :
        fname_(fn), srcloc_(srcloc)
    {}
    // Name of function whose definition lexically encloses the srcloc, or null.
    // This function is determined dynamically (taken from the stack frame).
    // In theory, we could have statically associated the name of the enclosing
    // function to the srcloc at parse or analysis time. But that's inadequate
    // if there are several aliases referring to the same lexical function
    // definition. In that case, we want dynamic information, so we can report
    // the function name used in the function call.
    FName fname_;
    // Source code location of a phrase that caused a run time panic, or
    // that was a function call on the stack at the time of the panic.
    // It may be a function call, in which case, the function is `fname_`.
    Src_Loc srcloc_;

    // Output a stack trace element, which is printed as part of
    // an exception message (no final newline). See Exception::write().
    // The `colour` flag enables colour text using ANSI ESC sequences.
    // The `many` flag is true if the stack trace has more than one element.
    void write(std::ostream&, bool colour, bool many) const;

    void write_json(std::ostream&) const;
};

} // namespace curv
#endif // header guard
