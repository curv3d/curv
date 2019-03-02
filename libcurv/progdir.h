// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_PROGDIR_H
#define LIBCURV_PROGDIR_H

#include <boost/filesystem.hpp>

namespace curv {

boost::filesystem::path progdir(const char* argv0);

}
#endif // header guard
