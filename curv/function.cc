// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/function.h>

void
curv::Function::print(std::ostream& out) const
{
    out << "<function>";
}

void
curv::Closure::print(std::ostream& out) const
{
    out << "<function>";
}
