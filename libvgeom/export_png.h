// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBVGEOM_EXPORT_PNG_H
#define LIBVGEOM_EXPORT_PNG_H

#include <libcurv/filesystem.h>

namespace vgeom {

struct Shape_Recognizer;

void export_png(Shape_Recognizer& shape, curv::Filesystem::path png_pathname);

} // namespace
#endif // header guard
