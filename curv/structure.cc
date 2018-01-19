// Copyright Doug Moen 2017-2018.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <curv/structure.h>
#include <curv/exception.h>

namespace curv {

const char Structure::name[] = "record";

Value
Structure::getfield(Atom field, const Context& cx) const
{
    throw Exception(cx, stringify(".",field,": not defined"));
}

} // namespace curv
