// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_SCRIPT_H
#define CURV_SCRIPT_H

#include <aux/range.h>
#include <curv/shared.h>
#include <string>

namespace curv {

/// Abstract base class: the name and contents of a script file.
///
/// The name is just an uninterpreted utf8 string for now, will later be
/// a filename or uri.
///
/// The contents are a const utf-8 character array.
/// Subclasses provide storage management for the contents:
/// eg, a std::string, a memory mapped file, or a GNU readline() buffer.
///
/// To use this class, you must define a subclass, and heap-allocate
/// instances using aux::make_shared.
struct Script : public aux::Shared_Base, public aux::Range<const char*>
{
    std::string name;
protected:
    Script(const std::string& nm, const char*f, const char*l)
    :
        Range(f,l), name(nm)
    {}
public:
    virtual ~Script() {}
};

} // namespace curv
#endif // header guard
