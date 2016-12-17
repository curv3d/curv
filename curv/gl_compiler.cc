// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/gl_compiler.h>
#include <curv/exception.h>
#include <curv/context.h>

void curv::gl_compile(const Shape2D&, std::ostream&)
{
    throw Exception({}, "Geometry Compiler not implemented");
}
