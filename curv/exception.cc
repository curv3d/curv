// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/exception.h>
#include <sstream>
#include <boost/format.hpp>

using namespace curv;

void
Char_Error::write(std::ostream& out) const
{
    out << message_ << " ";
    char ch = *loc_.range().begin();
    if (ch > 0x20 && ch < 0x7F)
        out << "'" << ch << "'";
    else
        out << boost::format("0x%X") % (unsigned)(unsigned char)ch;
    out << ": file " << loc_.scriptname()
        << ", line " << loc_.lineno();
}

void
Exception::write(std::ostream& out) const
{
    out << message_ << ": file " << loc_.scriptname()
        << ", line " << loc_.lineno()
        << ", token ";
    loc_.token_.write(out, loc_.script_);
}
