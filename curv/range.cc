// Copyright Doug Moen 2016-2018.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/range.h>

namespace curv {

std::ostream&
operator<<(std::ostream& out, const Range<const char*>& r)
{
    out.write(r.begin(), r.size());
    return out;
}

}
