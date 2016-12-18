// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/shape.h>
#include <curv/exception.h>
#include <curv/context.h>

using namespace curv;

void
Shape2D::print(std::ostream& out) const
{
    out << "shape2d";
    record_->print(out);
}

Value
Shape2D::getfield(Atom name) const
{
    return record_->getfield(name);
}

GL_Value
Shape2D::gl_dist(GL_Value arg, GL_Compiler& gl) const
{
    throw Exception({}, "Shape2D::gl_dist not implemented");
}
