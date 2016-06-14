// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef AUX_EXCEPTION_H
#define AUX_EXCEPTION_H

#include <exception>
#include <ostream>
#include <curv/string.h>

namespace aux {

/// Wrapper around std::exception that supports dynamic message strings
/// and context information.
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
/// aux::Exception provides a virtual `write` function which prints both
/// the message string and the context.
///
/// If a client wants to print a aux::Exception with full context, it needs
/// to catch an aux::Exception explicitly, and call write(). If it just
/// catches std::exception and prints the what() string, it will get only
/// the message string.
///
/// ## Dynamic Message Strings
///
/// aux::Exception allows the message string to be constructed dynamically
/// when the exception is thrown, rather than forcing it to be a string literal.
/// That's tricky, because an exception's copy constructor can't throw an
/// exception. You can't use a data member of type std::string. You need to
/// use some kind of shared_ptr.
///
/// aux::Exception represents the message string internally as
/// a `Shared_Ptr<curv::String>`. There is a convenient API, curv::stringify(),
/// for constructing a multi-component message string using a single function
/// call. [Yeah, this looks NIH, I could have used some combination of std::
/// and boost::. The requirements of the Curv project take precedence for now.]
///
/// ## Caveats
/// * The message string is dynamically allocated when the Exception is
///   thrown. In theory, this could lead to bad_alloc() being thrown instead
///   of the exception you were trying to throw. In practice, it's rare to
///   run out of heap space, and when you do, you are hosed: behaviour is
///   unpredictable anyway (on a modern virtual-memory machine--embedded
///   systems with 8 bit processors are a different problem space, and not
///   supported by this software).
/// * The what() string is no longer valid after the Exception is destroyed.
///   That is permitted by the C standard for std::exception. But I can imagine
///   somebody writing code that captures the what() pointer and preserves it
///   after the exception handler goes out of scope. That won't work here.
struct Exception : public std::exception
{
    Exception(const char* msg);
    Exception(Shared_Ptr<curv::String>);
    virtual void write(std::ostream&) const;
    virtual const char* what() const noexcept;
    Shared_Ptr<curv::String> const shared_what() { return message_; }
private:
    Shared_Ptr<curv::String> message_;
};

inline
std::ostream& operator<<(std::ostream& out, const aux::Exception& exc)
{
    exc.write(out);
    return out;
}

} // namespace aux
#endif // header guard
