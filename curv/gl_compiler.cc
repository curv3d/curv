// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/gl_compiler.h>
#include <curv/exception.h>
#include <curv/context.h>
#include <curv/shape.h>
#include <curv/meaning.h>

using namespace curv;

void curv::gl_compile(const Shape2D& shape, std::ostream& out)
{
    GL_Compiler gl(out);
    GL_Value dist_param = gl.newvalue(GL_Type::vec2);

    out <<
        "float main_dist(vec2 " << dist_param << ")\n"
        "{\n";

    GL_Value result = shape.gl_dist(dist_param, gl);

    out <<
        "  return " << result << ";\n"
        "}\n"
        "// minX, minY, maxX, maxY\n"
        "const vec4 bbox = vec4(-10.0,-10.0,+10.0,+10.0);\n"
        "void mainImage( out vec4 fragColor, in vec2 fragCoord )\n"
        "{\n"
        "    // transform `fragCoord` from viewport to world coordinates\n"
        "    float scale = (bbox.w-bbox.y)/iResolution.y;\n"
        "    float d = main_dist(fragCoord.xy*scale+bbox.xy);\n"
        "    if (d < 0.0)\n"
        "        fragColor = vec4(0,0,0,0);\n"
        "    else {\n"
        "        vec2 uv = fragCoord.xy / iResolution.xy;\n"
        "        fragColor = vec4(uv,0.5+0.5*sin(iGlobalTime),1.0);\n"
        "    }\n"
        "}\n"
        ;
}


GL_Value Operation::gl_eval(GL_Compiler&) const
{
    throw Exception(At_Phrase(*source_, nullptr),
        "this operation is not supported by the Geometry Compiler");
}

GL_Value Constant::gl_eval(GL_Compiler& gl) const
{
    if (value_.is_num()) {
        GL_Value result = gl.newvalue(GL_Type::num);
        double num = value_.get_num_unsafe();
        gl.out << "  float " << result << " = ";
        if (num == 1.0/0.0) // infinity
            gl.out << "1e999";
        else if (num == -1.0/0.0) // -infinity
            gl.out << "-1e999";
        else
            gl.out << value_;
        gl.out << ";\n";
        return result;
    }
    throw Exception(At_Phrase(*source_, nullptr),
        stringify("value ",value_," is not supported by the Geometry Compiler"));
}
