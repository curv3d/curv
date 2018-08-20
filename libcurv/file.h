// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_FILE_H
#define LIBCURV_FILE_H

#include <libcurv/source.h>

namespace curv {

struct Context;

/// A concrete Source class that represents a file.
struct Source_File : public Source_String
{
    // TODO: Should Source_File use mmap() to load the file into memory?
    Source_File(Shared<const String> filename, const Context&);
};

} // namespace curv
#endif // header guard
