// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_TEMPFILE_H
#define LIBCURV_GEOM_TEMPFILE_H

#include <libcurv/filesystem.h>

namespace curv { namespace geom {

Filesystem::path tempfile_name(const char* suffix);
Filesystem::path register_tempfile(const char* suffix);
//Filesystem::path make_tempfile(const char* suffix);
void remove_all_tempfiles();

}} // namespace
#endif // include guard
