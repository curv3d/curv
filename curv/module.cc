// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/module.h>

void
curv::Module::print(std::ostream& out) const
{
    out << "{";
    for (auto f : *this) {
        out << f.first << "=";
        f.second.print(out);
        out << ";";
    }
    for (auto e : *elements_) {
        out << e << ";";
    }
    out << "}";
}
