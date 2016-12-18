// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/gl_compiler.h>
#include <curv/exception.h>
#include <curv/context.h>
#include <curv/shape.h>

void curv::gl_compile(const Shape2D& shape, std::ostream& out)
{
    GL_Compiler gl(out);
    GL_Value dist_param = gl.newvalue(GL_Type::vec2);

    out <<
        "float main_dist(vec2 " << dist_param << ")\n"
        "{\n";

    GL_Value result = shape.gl_dist(dist_param, gl);

    out <<
        "return " << result << ";\n"
        "}\n";
}
