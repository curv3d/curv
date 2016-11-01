// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_EXCEPTION_H
#define CURV_EXCEPTION_H

#include <list>
#include <aux/exception.h>
#include <ostream>
#include <curv/location.h>
#include <curv/phrase.h>

namespace curv {

/// A Context describes the source code Location and call stack
/// of a compile-time or run time error.
///
/// More specifically, Context is an abstract interface to an object that
/// converts compile-time or run-time data structures into a list of Locations.
///
/// Context objects are not allocated on the heap, and don't own the data
/// structures that they point to. Instances of Context subclasses are
/// constructed in argument position as rvalues, are passed as `const Context&`
/// parameters to functions that may throw a curv::Exception, and are finally
/// consumed by the curv::Exception constructor.
///
/// An empty Context argument is `{}`.
/// Subclasses of Context are used for non-empty contexts.
struct Context
{
    virtual void get_locations(std::list<Location>&) const {}
};

/// Virtual base class for Curv compile time and run time errors.
///
/// Has two parts: a message (returned by what()), and a location() that
/// specifies where the error occurred.
/// These two parts are printed separately (see write()).
struct Exception : public aux::Exception
{
    // TODO: use std::shared_ptr so copy-ctor doesn't throw?
    std::list<Location> loc_;

    Exception(Location loc, const char* msg)
    : aux::Exception(msg), loc_({std::move(loc)}) {}

    Exception(Location loc, Shared<const String> msg)
    : aux::Exception(std::move(msg)), loc_({std::move(loc)}) {}

    Exception(const Context& cx, const char* msg)
    : aux::Exception(msg)
    {
        cx.get_locations(loc_);
    }

    Exception(const Context& cx, Shared<const String> msg)
    : aux::Exception(msg)
    {
        cx.get_locations(loc_);
    }

    //const Location& location() { return loc_; }

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
