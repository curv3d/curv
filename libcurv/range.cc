// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/range.h>

namespace curv {

std::ostream&
operator<<(std::ostream& out, const Range<const char*>& r)
{
    out.write(r.begin(), r.size());
    return out;
}

}
