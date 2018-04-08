// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef CURV_FILE_H
#define CURV_FILE_H

#include <curv/script.h>

namespace curv {

struct Context;

/// A concrete Script class that represents a file.
struct File_Script : public String_Script
{
    // TODO: Should File_Script use mmap() to load the file into memory?
    File_Script(Shared<const String> filename, const Context&);
};

} // namespace curv
#endif // header guard
