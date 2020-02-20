// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/format.h>

namespace curv {

// # of decimal digits in n
unsigned
ndigits(unsigned n)
{
    unsigned count = 1;
    for (;;) {
        if (n < 10)
            return count;
        n = n / 10;
        ++count;
    }
}

} // namespace
