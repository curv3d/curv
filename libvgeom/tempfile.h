// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBVGEOM_TEMPFILE_H
#define LIBVGEOM_TEMPFILE_H

#include <libcurv/filesystem.h>

namespace vgeom {

curv::Filesystem::path tempfile_name(const char* suffix);
void register_tempfile(const char* suffix);
curv::Filesystem::path make_tempfile(const char* suffix);
void remove_all_tempfiles();

} // namespace
#endif // include guard
