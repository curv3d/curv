// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_TEMPFILE_H
#define LIBCURV_GEOM_TEMPFILE_H

#include <libcurv/filesystem.h>

namespace curv { namespace geom {

unsigned make_tempfile_id();
Filesystem::path register_tempfile(unsigned id, const char* suffix);
void deregister_tempfile(Filesystem::path name);
void remove_all_tempfiles();

}} // namespace
#endif // include guard
