// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/exception.h>
#include <curv/string.h>
#include <curv/context.h>
#include <sstream>
#include <boost/format.hpp>

using namespace curv;
using namespace aux;

namespace curv {

Exception::Exception(const Context& cx, const char* msg)
: aux::Exception(cx.rewrite_message(make_string(msg)))
{
    cx.get_locations(loc_);
}

Exception::Exception(const Context& cx, Shared<const String> msg)
: aux::Exception(cx.rewrite_message(std::move(msg)))
{
    cx.get_locations(loc_);
}

Shared<const String>
illegal_character_message(char ch)
{
    String_Builder msg;
    msg << "illegal character ";
    if (ch > 0x20 && ch < 0x7F)
        msg << "'" << ch << "'";
    else
        msg << boost::format("0x%X") % (unsigned)(unsigned char)ch;
    return msg.get_string();
}

void
Exception::write(std::ostream& out) const
{
    out << what();
    for (auto L : loc_) {
        out << "\n";
        L.write(out);
    }
}

}
