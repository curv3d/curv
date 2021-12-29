// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/hash.h>

namespace curv {

// from djb2 by Dan Bernstein.
size_t strhash(const char *str) noexcept
{
    size_t hash = 5381;

    while (int c = *(const unsigned char*)str) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
        ++str;
    }

    return hash;
}

} // namespace curv
