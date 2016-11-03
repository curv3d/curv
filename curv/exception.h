// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_EXCEPTION_H
#define CURV_EXCEPTION_H

#include <list>
#include <ostream>
#include <aux/exception.h>
#include <curv/location.h>

namespace curv {

class Context;

/// Virtual base class for Curv compile time and run time errors.
///
/// Has two parts: a message (returned by what()), and a location() that
/// specifies where the error occurred.
/// These two parts are printed separately (see write()).
struct Exception : public aux::Exception
{
    // TODO: use std::shared_ptr so copy-ctor doesn't throw?
    std::list<Location> loc_;

    Exception(const Context& cx, const char* msg);
    Exception(const Context& cx, Shared<const String> msg);

    /// write the message and location to a stream.
    ///
    /// Multiple lines may be written, but no final newline.
    virtual void write(std::ostream&) const override;
};

Shared<const String> illegal_character_message(char ch);

inline std::ostream& operator<<(std::ostream& out, const Exception& e)
{
    e.write(out);
    return out;
}

} // namespace curv
#endif // header guard
