// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/exception.h>

#include <libcurv/ansi_colour.h>
#include <libcurv/context.h>
#include <libcurv/string.h>
#include <boost/format.hpp>
#include <sstream>

namespace curv {

void
Exception_Base::write(std::ostream& out, bool) const
{
    out << *message_;
}

const char*
Exception_Base::what() const noexcept
{
    return message_->c_str();
}

Exception_Base::Exception_Base(String_Ref msg)
:
    message_(std::move(msg))
{
}

Exception::Exception(const Context& cx, String_Ref msg)
:
    Exception_Base(cx.rewrite_message(std::move(msg)))
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
Exception::write(std::ostream& out, bool colour) const
{
    if (colour) out << AC_MESSAGE;
    out << what();
    if (colour) out << AC_RESET;
    for (auto L : loc_) {
        out << "\n";
        L.write(out, colour, loc_.size() > 1);
    }
}

}
