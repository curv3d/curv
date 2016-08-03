// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

#include <curv/module.h>

void
curv::Module::print(std::ostream& out) const
{
    out << "{";
    for (auto f : fields_) {
        out << f.first << "=";
        f.second.print(out);
        out << ";";
    }
    for (auto e : *elements_) {
        out << e << ";";
    }
    out << "}";
}
