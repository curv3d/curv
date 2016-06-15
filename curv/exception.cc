// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/exception.h>
#include <curv/string.h>
#include <sstream>
#include <boost/format.hpp>

using namespace curv;
using namespace aux;

namespace curv {

Shared_Ptr<String>
char_error_message(char ch)
{
    String_Builder msg;
    msg << "illegal character ";
    if (ch > 0x20 && ch < 0x7F)
        msg << "'" << ch << "'";
    else
        msg << boost::format("0x%X") % (unsigned)(unsigned char)ch;
    return msg.get_string();
}

Char_Error::Char_Error(const Script& s, Token tok)
:
    Token_Error(s, tok, char_error_message(*Location(s,tok).range().begin()))
{}

void
Exception::write(std::ostream& out) const
{
    out << "ERROR: " << what() << "\n";
    loc_.write(out);
}

}
