// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef AUX_PROGDIR_H
#define AUX_PROGDIR_H

#include <boost/filesystem.hpp>

namespace aux {

boost::filesystem::path progdir(const char* argv0);

} // namespace aux
#endif // header guard

