// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/shape.h>

void
curv::Shape2D::print(std::ostream& out) const
{
    out << "shape2d";
    record_->print(out);
}

auto curv::Shape2D::getfield(Atom name) const
-> Value
{
    return record_->getfield(name);
}
