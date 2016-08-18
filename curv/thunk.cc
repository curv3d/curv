// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/thunk.h>

void
curv::Thunk::print(std::ostream& out) const
{
    out << "<thunk>";
}
