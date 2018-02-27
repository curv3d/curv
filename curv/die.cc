// Copyright Doug Moen 2016-2018.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

extern "C" {
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
}
#include <curv/die.h>

namespace curv {

void
die_impl(const char* file, const char* line, const char* msg)
{
    // No dependencies on high level buffered i/o, so if that system is
    // corrupted, this will still work.
    write(2, file, strlen(file));
    write(2, ":", 1);
    write(2, line, strlen(line));
    write(2, ": ", 2);
    write(2, msg, strlen(msg));
    write(2, "\n", 1);
    abort();
}

} // namespace curv
