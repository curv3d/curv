// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <aux/exception.h>

using namespace aux;

void
Exception::write(std::ostream& out) const
{
    out << *message_;
}

const char*
Exception::what() const noexcept
{
    return message_->c_str();
}

Exception::Exception(const char* msg)
:
    message_(curv::mk_string(msg))
{
}

Exception::Exception(Shared<const curv::String> msg)
:
    message_(std::move(msg))
{
}
