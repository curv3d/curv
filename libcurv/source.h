// Copyright 2016-2021 Doug Moen
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
/// Either the name or contents can be missing (but not both).
///
/// The name is just an uninterpreted utf8 string for now, will later be
/// a filename or uri. If there is no filename, then the name is a zero-length
/// string, which won't be reported in error messages.
///
/// The contents are a const utf-8 character array.
/// Subclasses provide storage management for the contents.
/// The contents can be null (different from zero length),
/// in which case error messages only report the file name.
struct Source : public Shared_Base, public Range<const char*>
{
    enum class Type { curv, gpu, directory };

    Shared<const String> name_;
    Type type_ = Type::curv;
protected:
    Source(String_Ref name, const char*f, const char*l)
    :
        Range(f,l), name_(move(name))
    {}
public:
    Source(String_Ref name, Type type = Type::curv)
    :
        Range(nullptr,nullptr), name_(move(name)), type_(type)
    {}
    bool no_name() const { return name_->empty(); }
    bool no_contents() const { return first == nullptr; }
    virtual ~Source() {}
};

/// A Source subclass where the program text is represented as a String.
struct String_Source : public Source
{
    Shared<const String> text_;

    String_Source(String_Ref name, String_Ref text)
    :
        Source(move(name), text->data(), text->data() + text->size()),
        text_(move(text))
    {}
};

/// A Source subclass that represents a file.
struct File_Source : public String_Source
{
    File_Source(String_Ref filename, const Context&);
};

} // namespace curv
#endif // header guard
