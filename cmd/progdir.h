// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef PROGDIR_H
#define PROGDIR_H

#include <boost/filesystem.hpp>

boost::filesystem::path progdir(const char* argv0);

#endif // header guard

