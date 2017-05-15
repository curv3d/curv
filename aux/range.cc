// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <aux/range.h>

std::ostream&
aux::operator<<(std::ostream& out, const aux::Range<const char*>& r)
{
    out.write(r.begin(), r.size());
    return out;
}
