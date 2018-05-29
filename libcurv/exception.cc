// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/exception.h>
#include <libcurv/string.h>
#include <libcurv/context.h>
#include <sstream>
#include <boost/format.hpp>

namespace curv {

void
Exception_Base::write(std::ostream& out) const
{
    out << *message_;
}

const char*
Exception_Base::what() const noexcept
{
    return message_->c_str();
}

Exception_Base::Exception_Base(const char* msg)
:
    message_(curv::make_string(msg))
{
}

Exception_Base::Exception_Base(curv::Shared<const curv::String> msg)
:
    message_(std::move(msg))
{
}

Exception::Exception(const Context& cx, const char* msg)
: Exception_Base(cx.rewrite_message(make_string(msg)))
{
    cx.get_locations(loc_);
}

Exception::Exception(const Context& cx, Shared<const String> msg)
: Exception_Base(cx.rewrite_message(std::move(msg)))
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
