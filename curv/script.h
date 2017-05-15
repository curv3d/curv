// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_SCRIPT_H
#define CURV_SCRIPT_H

#include <aux/range.h>
#include <curv/shared.h>
#include <curv/string.h>

namespace curv {

/// Abstract base class: the name and contents of a script file.
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
struct Script : public aux::Shared_Base, public aux::Range<const char*>
{
    Shared<const String> name_;
protected:
    Script(Shared<const String> name, const char*f, const char*l)
    :
        Range(f,l), name_(std::move(name))
    {}
public:
    virtual ~Script() {}
};

/// A concrete Script subclass where the contents are represented as a String.
struct String_Script : public curv::Script
{
    Shared<const String> buffer_;

    String_Script(Shared<const String> name, Shared<const String> buffer)
    :
        curv::Script(
            std::move(name), buffer->data(), buffer->data() + buffer->size()),
        buffer_(std::move(buffer))
    {}
};

} // namespace curv
#endif // header guard
