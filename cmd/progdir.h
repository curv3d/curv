// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#ifndef PROGDIR_H
#define PROGDIR_H

#include <boost/filesystem.hpp>

boost::filesystem::path progdir(const char* argv0);

#endif // header guard

