// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef CURV_EXCEPTION_H
#define CURV_EXCEPTION_H

#include <exception>
#include <list>
#include <ostream>
#include <curv/location.h>
#include <curv/string.h>

namespace curv {

/// Exception_Base: Wrapper around std::exception that supports dynamic message
/// strings and context information.
///
/// ## Context Information
///
/// C++ exception objects generally contain two kinds of information:
/// * a message string, returned by std::exception::what()
/// * additional data which describes the context of the exception
///   (eg, location in source code)
///
/// You can't put all of the context information in the what() string,
/// because in general, that would require what() to dynamically allocate
/// a new string, and what() is noexcept. (Constructing the what() string
/// before constructing the exception object is not generally possible
/// if your code adds new context information to the exception before it
/// is printed, by catching an exception, adding context, and rethrowing it.)
///
/// Therefore, the context information must be separate from the what() string.
/// And there needs to be a generic API for printing this information.
/// Exception_Base provides a virtual `write` function which prints both
/// the message string and the context.
///
/// If a client wants to print an Exception_Base with full context, it needs
/// to catch an Exception_Base explicitly, and call write(). If it just
/// catches std::exception and prints the what() string, it will get only
/// the message string.
///
/// ## Dynamic Message Strings
///
/// Exception_Base allows the message string to be constructed dynamically
/// when the exception is thrown, rather than forcing it to be a string literal.
/// That's tricky, because an exception's copy constructor can't throw an
/// exception. You can't use a data member of type std::string. You need to
/// use some kind of shared_ptr.
///
/// Exception_Base represents the message string internally as
/// a `Shared<String>`. There is a convenient API, curv::stringify(),
/// for constructing a multi-component message string using a single function
/// call. [Yeah, this looks NIH, I could have used some combination of std::
/// and boost::. The requirements of the Curv project take precedence for now.]
///
/// ## Caveats
/// * The message string is dynamically allocated when the Exception_Base is
///   thrown. In theory, this could lead to bad_alloc() being thrown instead
///   of the exception you were trying to throw. In practice, it's rare to
///   run out of heap space, and when you do, you are hosed: behaviour is
///   unpredictable anyway (on a modern virtual-memory machine--embedded
///   systems with 8 bit processors are a different problem space, and not
///   supported by this software).
/// * The what() string is no longer valid after the Exception_Base is destroyed.
///   That is permitted by the C standard for std::exception. But I can imagine
///   somebody writing code that captures the what() pointer and preserves it
///   after the exception handler goes out of scope. That won't work here.
struct Exception_Base : public std::exception
{
    Exception_Base(const char* msg);
    Exception_Base(Shared<const String>);
    virtual void write(std::ostream&) const;
    virtual const char* what() const noexcept;
    Shared<const String> const shared_what() { return message_; }
private:
    Shared<const String> message_;
};

inline
std::ostream& operator<<(std::ostream& out, const Exception_Base& exc)
{
    exc.write(out);
    return out;
}

struct Context;

/// Virtual base class for Curv compile time and run time errors.
///
/// Has two parts: a message (returned by what()), and a location() that
/// specifies where the error occurred.
/// These two parts are printed separately (see write()).
struct Exception : public Exception_Base
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
