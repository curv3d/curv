// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SOURCE_H
#define LIBCURV_SOURCE_H

#include <libcurv/range.h>
#include <libcurv/shared.h>
#include <libcurv/string.h>

namespace curv {

/// Abstract base class: the name and contents of a source file.
///
/// The name is just an uninterpreted utf8 string for now, will later be
/// a filename or uri. If there is no filename, then the name is a zero-length
/// string, which won't be reported in error messages.
///
/// The contents are a const utf-8 character array.
/// Subclasses provide storage management for the contents:
/// eg, a std::string, a memory mapped file, or a GNU readline() buffer.
///
/// To use this class, you must define a subclass, and heap-allocate
/// instances using make.
struct Source : public Shared_Base, public Range<const char*>
{
    Shared<const String> name_;
protected:
    Source(Shared<const String> name, const char*f, const char*l)
    :
        Range(f,l), name_(std::move(name))
    {}
public:
    virtual ~Source() {}
};

/// A concrete Source subclass where the contents are represented as a String.
struct Source_String : public curv::Source
{
    Shared<const String> buffer_;

    Source_String(Shared<const String> name, Shared<const String> buffer)
    :
        curv::Source(
            std::move(name), buffer->data(), buffer->data() + buffer->size()),
        buffer_(std::move(buffer))
    {}
};

} // namespace curv
#endif // header guard
