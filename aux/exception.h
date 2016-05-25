// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef AUX_EXCEPTION_H
#define AUX_EXCEPTION_H

#include <exception>
#include <ostream>

namespace aux {

/// Wrapper around std::exception, with an interface for printing message and
/// details to a stream.
///
/// In general, subclasses of aux::Exception contain data which describe
/// the context of the exception (eg, location in source code).
/// * This data can be copied without throwing an exception. The C++ exception
///   mechanism sometimes copies exception values, and it would be bad if the
///   copy constructor threw an exception during exception processing.
/// * Aux exceptions can be printed: this produces formatted output that
///   includes the context of the exception. Subclasses of aux::Exception
///   implement the write() function to produce this formatted message.
///
/// aux::Exception is a subclass of std::exception so that clients of the
/// aux library can catch std::exception, and catch exceptions from all
/// libraries, including aux exceptions.
///
/// aux::Exception is a distinct base class so that clients can choose
/// to selectively catch all Aux exceptions.
///
/// std::exception::what() is a problem. At first glance, it should return
/// the fully formatted message, including the context. But that turns out to
/// be excessively challenging, because that would require dynamic string
/// allocation and ownership of the memory, which is complicated by many things:
/// * The constructor for an exception class shouldn't throw an exception,
///   it leads to confusing behaviour (wrong exception thrown by a throw).
/// * The copy constructor can't throw an exception (forbidden by C++ standard).
///   Storing a std::string as a data member is ruled out.
///   See also http://www.boost.org/community/error_handling.html
///   Note that shared_ptr<string> could work.
/// * `what()` itself can't throw an exception. This is the best place to
///   generate the formatted message, but the required memory allocation needs
///   to be guarded by try/catch, and what string do I return if the formatting
///   fails?
/// * What is the lifetime of the string returned by what()? The standard
///   says the value remains valid until the exception is destroyed.
///   Any memory allocated by `what()` needs to be owned by the exception value.
///   Also, storing the newly allocated string back into the exception seems to
///   conflict with the standard, which defines `what()` to be const.
///   (The data member could be declared `mutable`, but how does that affect the
///   exception mechanism?)
///   Also, the common implementation of `what()` is a static string, and there
///   may be client code that assumes this by referencing the string after the
///   exception is destroyed.
///
/// My conclusion is that `what()` should only return a static string, so this
/// string is stored in aux::Exception as a data member.
/// If a client wants to print a aux::Exception with full context, it needs
/// to catch a aux::Exception explicitly, and call write(). If it just
/// catches std::exception and prints the what() string, it will get only
/// the static part of the message.
struct Exception : public std::exception
{
    Exception(const char* msg) : message_(msg) {}
    virtual void write(std::ostream&) const;
    virtual const char* what() const noexcept;
    const char* message_;
};

inline
std::ostream& operator<<(std::ostream& out, const aux::Exception& exc)
{
    exc.write(out);
    return out;
}

} // namespace aux
#endif // header guard
