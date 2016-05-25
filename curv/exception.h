// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_EXCEPTION_H
#define CURV_EXCEPTION_H

#include <aux/exception.h>
#include <ostream>
#include <curv/token.h>

namespace curv {

/// Virtual base class for Curv compile time and run time errors.
///
/// Has a precise source code location (where the error occurred),
/// represented as a range of characters within a specific script.
/// Using this interface, a CLI tool can report an error by printing
/// a filename and line number, and an IDE can highlight the text range
/// containing the error.
struct Exception : public aux::Exception
{
    Exception(const char* msg) : aux::Exception(msg) {}
#if 0
    virtual void write(std::ostream&) const;

    /// In which script did the error occur? (implemented by subclass)
    virtual Script& script() const = 0;

    /// In which character range did the error occur? (implemented by subclass)
    virtual Token location() const = 0;

    /// Name of script where error occurred.
    const std::string& scriptname() const { return script()->name; }

    /// Line number within script where error occurred.
    int lineno() const { return location()->lineno(script()); }

    /// Range of characters within script where error occurred.
    Range<const char*> range() const { return location()->range(script()); }
#endif
};

/// error containing a token (full source code location) + a message string
struct SyntaxError : public Exception
{
    SyntaxError(Token tok, const char* msg)
    : Exception(msg), token_(std::move(tok))
    {}
    Token token_;
    virtual void write(std::ostream&) const;
};

/// Lexical analysis error: an illegal character in the input.
///
/// Subclass of SyntaxError where the token spans just the illegal character.
struct BadCharacter : public SyntaxError
{
    BadCharacter(Token tok)
    : SyntaxError(std::move(tok), "illegal character")
    {}
    virtual void write(std::ostream&) const;
};

} // namespace curv
#endif // header guard
