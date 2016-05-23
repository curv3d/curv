// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#ifndef CURV_SCRIPT_H
#define CURV_SCRIPT_H

#include <aux/range.h>
#include <string>

namespace curv {

/// \brief Abstract base class: the name and contents of a script file.
///
/// The name is just an uninterpreted utf8 string for now, will later be
/// a filename or uri.
///
/// The contents are a const utf-8 character array.
/// Subclasses provide storage management for the contents:
/// eg, a std::string, or a memory mapped file.
struct Script : public aux::Range<const char*>
{
    Script(const std::string& nm, const char*f, const char*l)
    :
        name(nm), Range(f,l)
    {}
    std::string name;
};

} // namespace curv
#endif // header guard
