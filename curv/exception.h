// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_EXCEPTION_H
#define CURV_EXCEPTION_H

#include <aux/exception.h>
#include <ostream>
#include <curv/location.h>
#include <curv/syntax.h>

namespace curv {

/// Virtual base class for Curv compile time and run time errors.
///
/// Has a precise source code location (where the error occurred).
struct Exception : public aux::Exception
{
    Location loc_;

    Exception(Location loc, const char* msg)
    : aux::Exception(msg), loc_(std::move(loc)) {}

    const Location& location() { return loc_; }

    virtual void write(std::ostream&) const override;
};

/// Curv error, where location is specified by a token.
struct Token_Error : public Exception
{
    Token_Error(const Script& s, Token tok, const char* msg)
    : Exception(Location(s, std::move(tok)), msg)
    {}
};

/// Lexical analysis error: an illegal character in the input.
///
/// Subclass of Token_Error where the token spans just the illegal character.
struct Char_Error : public Token_Error
{
    Char_Error(const Script& s, Token tok)
    : Token_Error(s, std::move(tok), "illegal character")
    {}
    virtual void write(std::ostream&) const;
};

struct Syntax_Error : public Exception
{
    Syntax_Error(const Syntax& syn, const char* msg)
    : Exception(syn.location(), msg)
    {}
};

} // namespace curv
#endif // header guard
