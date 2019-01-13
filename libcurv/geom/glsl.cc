// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/glsl.h>

#include <libcurv/geom/shape.h>
#include <libcurv/geom/viewed_shape.h>

#include <libcurv/context.h>
#include <libcurv/die.h>
#include <libcurv/dtostr.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/gl_compiler.h>
#include <libcurv/gl_context.h>

namespace curv { namespace geom {

void glsl_function_export(const Shape_Program& shape, std::ostream& out)
{
    GL_Compiler gl(out, GL_Target::glsl, shape.system());

    if (shape.viewed_shape_) {
        // output uniform variables for parametric shape
        for (auto& p : shape.viewed_shape_->param_) {
            out << "uniform " << p.second.pconfig_.gltype_
                << " rv_" << p.first << ";\n";
        }
    }

    GL_Value dist_param = gl.newvalue(GL_Type::Vec(4));
    out <<
        "float dist(vec4 " << dist_param << ")\n"
        "{\n";
    GL_Value dist_result = shape.gl_dist(dist_param, gl);
    out <<
        "  return " << dist_result << ";\n"
        "}\n";

    GL_Value colour_param = gl.newvalue(GL_Type::Vec(4));
    out <<
        "vec3 colour(vec4 " << colour_param << ")\n"
        "{\n";
    GL_Value colour_result = shape.gl_colour(colour_param, gl);
    out <<
        "  return " << colour_result << ";\n"
        "}\n";
}

}} // namespace
