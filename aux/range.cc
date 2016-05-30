// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <aux/range.h>

std::ostream&
aux::operator<<(std::ostream& out, const aux::Range<const char*>& r)
{
    out.write(r.begin(), r.size());
    return out;
}
