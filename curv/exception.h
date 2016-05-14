/*
 * Copyright 2016 Doug Moen. See LICENCE.md file for terms of use.
 */
#ifndef CURV_EXCEPTION_H
#define CURV_EXCEPTION_H

#include <exception>
#include <ostream>
#include <curv/token.h>

namespace curv {

/// Virtual base class for Curv compile time and run time errors.
///
/// In general, subclasses of curv::Exception contain data which describe
/// the context of the exception (eg, location in source code).
/// * This data can be copied without throwing an exception. The C++ exception
///   mechanism sometimes copies exception values, and it would be bad if the
///   copy constructor threw an exception during exception processing.
/// * Curv exceptions can be printed: this produces formatted output that
///   includes the context of the exception. Subclasses of curv::Exception
///   implement the write() function to produce this formatted message.
///
/// curv::Exception is a subclass of std::exception so that clients of the
/// curv library can catch std::exception, and catch exceptions from all
/// libraries, including curv exceptions.
///
/// curv::Exception is a distinct base class so that clients can choose
/// to selectively catch all Curv exceptions.
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
///   shared_ptr<string> could work, but that adds a layer of complexity.
/// * `what()` itself can't throw an exception. This is the best place to
///   generate the formatted message, but the required memory allocation needs
///   to be guarded by try/catch, and what string do I return if the formatting
///   fails?
/// * What is the lifetime of the string returned by what()? The standard
///   says the value remains valid until the exception is destroyed.
///   Any memory allocated by `what()` needs to be owned by the exception value,
///   but now it seems I need a std::string data member, see above.
///   Also, storing the newly allocated string back into the exception seems to
///   conflict with the standard, which defines `what()` to be const.
///   Also, the common implementation of `what()` is a static string, and there
///   may be client code that assumes this by referencing the string after the
///   exception is destroyed.
///
/// My conclusion is that `what()` should only return a static string, so this
/// string is stored in curv::Exception as a data member.
/// If a client wants to print a curv::Exception with full context, it needs
/// to catch a curv::Exception explicitly, and call write(). If it just
/// catches std::exception and prints the what() string, it will get only
/// the static part of the message.
struct Exception : public std::exception
{
    Exception(const char* msg) : message_(msg) {}
    virtual void write(std::ostream&) const;
    virtual const char* what() const noexcept;
    const char* message_;
};

/// error containing a token (full source code location) + a message string
struct SyntaxError : public Exception
{
    SyntaxError(Token tok, const char* msg)
    : Exception(msg), token_(std::move(tok))
    {}
    Token token_;
    virtual void write(std::ostream&) const;
};

inline
std::ostream& operator<<(std::ostream& out, const curv::Exception& exc)
{
    exc.write(out);
    return out;
}

} // namespace curv
#endif // header guard
