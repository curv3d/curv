// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_FILE_H
#define CURV_FILE_H

#include <curv/script.h>

namespace curv {

class Context;

/// A concrete Script class that represents a file.
struct File_Script : public String_Script
{
    // TODO: Should File_Script use mmap() to load the file into memory?
    File_Script(Shared<const String> filename, const Context&);
};

} // namespace curv
#endif // header guard
