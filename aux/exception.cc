// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <aux/exception.h>

using namespace aux;

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
