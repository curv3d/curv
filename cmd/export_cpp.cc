// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include "export.h"
#include <fstream>
#include <curv/exception.h>
#include <curv/shape.h>

void export_cpp(curv::Value value,
    curv::System& sys, const curv::Context& cx, const Export_Params&,
    std::ostream& out)
{
    curv::Shape_Recognizer shape(cx, sys);
    if (!shape.recognize(value))
        throw curv::Exception(cx, "not a shape");

    curv::GL_Compiler gl(out, curv::GL_Target::cpp);
    curv::GL_Value dist_param = gl.newvalue(curv::GL_Type::Vec4);

    out <<
        "#include <glm/vec2.hpp>\n"
        "#include <glm/vec3.hpp>\n"
        "#include <glm/vec4.hpp>\n"
        "#include <glm/detail/func_common.hpp>\n"
        "#include <glm/detail/func_geometric.hpp>\n"
        "#include <glm/detail/func_trigonometric.hpp>\n"
        "\n"
        "using namespace glm;\n"
        "\n"

        "float dist(vec4 " << dist_param << ")\n"
        "{\n";
    curv::GL_Value result = shape.gl_dist(dist_param, gl);
    out <<
        "  return " << result << ";\n"
        "}\n";
}
