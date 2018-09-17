// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_CONTEXT_H
#define LIBCURV_CONTEXT_H

#include <list>
#include <libcurv/location.h>
#include <libcurv/frame.h>

namespace curv {

struct Phrase;
struct Environ;
struct Scanner;
struct Function;
struct System;

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
    virtual ~Context() {}
    virtual void get_locations(std::list<Location>&) const = 0;
    virtual Shared<const String> rewrite_message(Shared<const String>) const;
};

struct At_System : public Context
{
    System& system_;

    At_System(System& system) : system_(system) {}

    virtual void get_locations(std::list<Location>& locs) const;
};

struct At_Frame : public Context
{
    Frame& call_frame_;

    At_Frame(Frame& frame) : call_frame_(frame) {}

    virtual void get_locations(std::list<Location>& locs) const;
};

void get_frame_locations(const Frame* f, std::list<Location>& locs);

struct At_Token : public Context
{
    Location loc_;
    System& system_;
    Frame* file_frame_;

    At_Token(Token, const Scanner&);
    At_Token(Token, const Phrase&, Environ&);
    At_Token(Location, Environ&);
    At_Token(Location, System&, Frame* = nullptr);

    virtual void get_locations(std::list<Location>&) const;
};

/// Exception Context where we know the Phrase that contains the error.
struct At_Phrase : public Context
{
    const Phrase& phrase_;
    System& system_;
    Frame* frame_; // file_frame or call_frame

    At_Phrase(const Phrase& phrase, Frame& call_frame);
    At_Phrase(const Phrase& phrase, System& sys, Frame* frame);
    At_Phrase(const Phrase& phrase, Scanner& scanner);
    At_Phrase(const Phrase& phrase, Environ& env);

    virtual void get_locations(std::list<Location>& locs) const;
};

struct At_Program : public At_Token
{
    // works with curv::Program or curv::geom::Shape_Program
    template <class PROGRAM>
    explicit At_Program(const PROGRAM& prog)
    :
        At_Token(prog.location(), prog.system(), prog.file_frame())
    {}
};

// Bad argument to a function call.
struct At_Arg : public Context
{
    Function& fun_;
    Frame& call_frame_;

    At_Arg(Function& fn, Frame& fr)
    :
        fun_(fn),
        call_frame_(fr)
    {}

    void get_locations(std::list<Location>& locs) const;
    virtual Shared<const String> rewrite_message(Shared<const String>) const override;
};

// Bad argument to a metafunction call.
struct At_Metacall : public Context
{
    const char* name_;
    unsigned argpos_;
    const Phrase& arg_;
    Frame& call_frame_;

    At_Metacall(const char* name, unsigned argpos, const Phrase& arg, Frame& fr)
    :
        name_(name),
        argpos_(argpos),
        arg_(arg),
        call_frame_(fr)
    {}

    void get_locations(std::list<Location>& locs) const;
    virtual Shared<const String> rewrite_message(Shared<const String>) const override;
};

struct At_Field : public Context
{
    const char* fieldname_;
    const Context& parent_;

    At_Field(const char* fieldname, const Context& parent);

    virtual void get_locations(std::list<Location>&) const;
    virtual Shared<const String> rewrite_message(Shared<const String>) const override;
};

struct At_Index : public Context
{
    size_t index_;
    const Context& parent_;

    At_Index(size_t index, const Context& parent);

    virtual void get_locations(std::list<Location>&) const;
    virtual Shared<const String> rewrite_message(Shared<const String>) const override;
};

} // namespace curv
#endif // header guard
