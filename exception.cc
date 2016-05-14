/*
 * Copyright 2016 Doug Moen. See LICENCE.md file for terms of use.
 */
#include <curv/exception.h>
#include <sstream>

using namespace curv;

void
SyntaxError::write(std::ostream& out) const
{
    out << message_ << ": file " << token_.scriptname()
        << ", line " << token_.lineno()
        << ", token " << token_;
}

void
Exception::write(std::ostream& out) const
{
    out << message_;
}

const char*
Exception::what() const noexcept
{
    return message_;
}
