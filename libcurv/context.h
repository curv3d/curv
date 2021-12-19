// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_CONTEXT_H
#define LIBCURV_CONTEXT_H

#include <list>
#include <libcurv/frame.h>
#include <libcurv/location.h>

namespace curv {

struct Phrase;
struct Environ;
struct Scanner;
struct System;

/// The primary use of a Context is to describe the source code Location and
/// call stack of a compile-time or run time error. A Context reference is
/// passed as the first constructor argument to curv::Exception, which is used
/// for reporting errors and warnings. The Context is used to construct the
/// stack trace, via Context::get_locations().
///
/// Libcurv is written in a functional style. There are no global mutable
/// variables. Runtime functions in libcurv have no access to 'global'
/// interpreter state, exception via arguments that are passed in. Since you
/// need a Context to throw an exception, many runtime functions
/// have a Context argument, if they don't already have a Frame argument.
///
/// As a consequence of this coding style, the secondary use of a Context is
/// to gain access to the System object, and to the Frame at the top of the
/// evaluator stack, via Context::system() and Context::frame().
/// This is needed to construct a Program object for compiling a Curv source
/// file. This need can arise whenever the `lib` is passed around as a value,
/// due to lazy evaluation and lazy program loading in `lib`.
///
/// We only need Context::frame() at runtime: that's when `lib` expressions
/// are evaluated. During SC_Compile, Context::frame() returns nullptr.
///
/// Context objects must be very cheap to construct.
/// Therefore, they are small, with only a few fields to fill in, and with
/// little or no computation required in the constructor. There are many
/// Context subclasses, each tailored to a particular kind of context, where
/// different compile-time or run-time data is available. In each case, we
/// take whatever data structures are readily available, store references to
/// them in the Context object, and later (if an Exception is thrown),
/// then we perform additional and more expensive computation to construct
/// a stack trace. So construction is cheap, but Context::get_location() is
/// expensive.
///
/// Context objects are not allocated on the heap, and don't own the data
/// structures that they point to. Instances of Context subclasses are usually
/// constructed in argument position as rvalues, and passed as `const Context&`
/// parameters to functions that may throw a curv::Exception.
///
/// Context::frame() and Context::system() are a little more expensive than
/// the Context constructors, but that's okay, because we need them when
/// loading a source file into memory, which is already expensive.
struct Context
{
    virtual ~Context() {}
    virtual void get_locations(std::list<Func_Loc>&) const = 0;
    virtual Shared<const String> rewrite_message(Shared<const String>) const;
    virtual System& system() const = 0;
    virtual Frame* frame() const = 0;
};

void get_frame_locations(const Frame* f, std::list<Func_Loc>& locs);

struct At_System : public Context
{
    System& system_;

    At_System(System& system) : system_(system) {}

    virtual void get_locations(std::list<Func_Loc>& locs) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
};

struct At_SState : public Context
{
    Source_State& sstate_;

    At_SState(Source_State& ss) : sstate_(ss) {}

    virtual void get_locations(std::list<Func_Loc>& locs) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
};

struct At_Token : public Context
{
    Src_Loc loc_;
    System& system_;
    Frame* file_frame_;

    At_Token(Token, const Scanner&);
    At_Token(Token, const Phrase&, Environ&);
    At_Token(Token, const Phrase&, Frame&);
    At_Token(Src_Loc, Environ&);
    At_Token(Src_Loc, System&, Frame* = nullptr);

    virtual void get_locations(std::list<Func_Loc>&) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
};

// An abstract Context class which contains a const Phrase& reference,
// indicating the span of program text where the error occurred.
struct At_Syntax : public Context
{
    virtual const Phrase& syntax() const = 0;
};

struct At_Frame : public At_Syntax
{
    Frame& call_frame_;

    At_Frame(Frame& frame) : call_frame_(frame) {}

    virtual void get_locations(std::list<Func_Loc>& locs) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
    virtual const Phrase& syntax() const override;
};

/// Exception Context where we know the Phrase that contains the error.
struct At_Phrase : public At_Syntax
{
    Shared<const Phrase> phrase_;
    System& system_;
    Frame* frame_; // file_frame or call_frame

    At_Phrase(Shared<const Phrase> phrase, Frame& call_frame);
    At_Phrase(const Phrase& phrase, Frame& call_frame);
    At_Phrase(const Phrase& phrase, Source_State&);
    At_Phrase(const Phrase& phrase, Scanner& scanner);
    At_Phrase(const Phrase& phrase, Environ& env);

    virtual void get_locations(std::list<Func_Loc>& locs) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
    virtual const Phrase& syntax() const override;
};

struct At_Program : public At_Syntax
{
    Source_State& sstate_;
    const Phrase& syntax_;

    // works with curv::Program, curv::Shape_Program, curv::GPU_Program
    template <class PROGRAM>
    explicit At_Program(PROGRAM& prog)
    :
        sstate_(prog.sstate_), syntax_(prog.syntax())
    {}

    virtual void get_locations(std::list<Func_Loc>& locs) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
    virtual const Phrase& syntax() const override;
};

// Bad argument to a function call.
// The Frame argument is the stack frame for the function call.
struct At_Arg : public At_Syntax
{
    const Function& func_;
    Frame& call_frame_;

    At_Arg(const Function& fn, Frame& fm)
    :
        func_(fn),
        call_frame_(fm)
    {}

    void get_locations(std::list<Func_Loc>& locs) const override;
    virtual Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
    virtual const Phrase& syntax() const override;
};

// Bad argument to a metafunction call (Exception Context only).
// The Frame argument is the parent stack frame; the metafunction call doesn't
// have its own stack frame. This is good if the Context is just being used as
// an Exception context, but not good for more general uses, such as passing
// to an import function, where the frame() is accessed. For a more expensive
// but more general purpose Context, use At_Metacall_With_Call_Frame.
struct At_Metacall : public At_Syntax
{
    const char* name_;
    unsigned argpos_;
    const Phrase& arg_;
    Frame& parent_frame_;

    At_Metacall(const char* name, unsigned argpos, const Phrase& arg, Frame& fm)
    :
        name_(name),
        argpos_(argpos),
        arg_(arg),
        parent_frame_(fm)
    {}

    void get_locations(std::list<Func_Loc>& locs) const override;
    virtual Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
    virtual const Phrase& syntax() const override;
};

// Bad argument to a metafunction call (general purpose Context).
// The client must explicitly allocate a call frame for the metafunction call,
// and pass it as the frame argument.
struct At_Metacall_With_Call_Frame : public At_Syntax
{
    const char* name_;
    unsigned argpos_;
    Frame& call_frame_;

    At_Metacall_With_Call_Frame(const char* name, unsigned argpos, Frame& fm)
    :
        name_(name),
        argpos_(argpos),
        call_frame_(fm)
    {}

    void get_locations(std::list<Func_Loc>& locs) const override;
    virtual Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
    virtual const Phrase& syntax() const override;
};

// A convenience class, to automate the boilerplate of deriving from At_Syntax.
struct At_Syntax_Wrapper : public At_Syntax
{
    const At_Syntax& parent_;
    At_Syntax_Wrapper(const At_Syntax& cx) : parent_(cx) {}
    Shared<const String> rewrite_message(Shared<const String> s) const override;
    void get_locations(std::list<Func_Loc>& locs) const override;
    System& system() const override;
    Frame* frame() const override;
    const Phrase& syntax() const override;
};

struct At_Field_Syntax : public At_Syntax_Wrapper
{
    const char* fieldname_;

    At_Field_Syntax(const char* fieldname, const At_Syntax& parent);

    virtual Shared<const String> rewrite_message(Shared<const String>) const override;
};

struct At_Index_Syntax : public At_Syntax_Wrapper
{
    size_t index_;

    At_Index_Syntax(size_t index, const At_Syntax& parent);

    virtual Shared<const String> rewrite_message(Shared<const String>) const override;
};

struct At_Field : public Context
{
    const char* fieldname_;
    const Context& parent_;

    At_Field(const char* fieldname, const Context& parent);

    virtual void get_locations(std::list<Func_Loc>&) const override;
    virtual Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
};

struct At_Index : public Context
{
    size_t index_;
    const Context& parent_;

    At_Index(size_t index, const Context& parent);

    virtual void get_locations(std::list<Func_Loc>&) const override;
    virtual Shared<const String> rewrite_message(Shared<const String>) const override;
    virtual System& system() const override;
    virtual Frame* frame() const override;
};

} // namespace curv
#endif // header guard
