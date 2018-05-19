// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef TEMPFILE_H
#define TEMPFILE_H

#include <boost/filesystem.hpp>

boost::filesystem::path tempfile_name(const char* suffix);
void register_tempfile(const char* suffix);
boost::filesystem::path make_tempfile(const char* suffix);
void remove_all_tempfiles();

#endif // include guard
