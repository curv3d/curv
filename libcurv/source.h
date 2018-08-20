// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SOURCE_H
#define LIBCURV_SOURCE_H

#include <libcurv/range.h>
#include <libcurv/shared.h>
#include <libcurv/string.h>

namespace curv {

struct Context;

/// Abstract base class: the name and contents of a source file.
///
/// The name is just an uninterpreted utf8 string for now, will later be
/// a filename or uri. If there is no filename, then the name is a zero-length
/// string, which won't be reported in error messages.
///
/// The contents are a const utf-8 character array.
/// Subclasses provide storage management for the contents.
struct Source : public Shared_Base, public Range<const char*>
{
    Shared<const String> name_;
protected:
    Source(String_Ref name, const char*f, const char*l)
    :
        Range(f,l), name_(std::move(name))
    {}
public:
    virtual ~Source() {}
};

/// A Source subclass where the program text is represented as a String.
struct Source_String : public Source
{
    Shared<const String> text_;

    Source_String(String_Ref name, String_Ref text)
    :
        Source(std::move(name), text->data(), text->data() + text->size()),
        text_(std::move(text))
    {}
};

/// A Source subclass that represents a file.
struct Source_File : public Source_String
{
    Source_File(String_Ref filename, const Context&);
};

} // namespace curv
#endif // header guard
