// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/glsl.h>

#include <libcurv/context.h>
#include <libcurv/function.h>
#include <libcurv/gl_compiler.h>
#include <libcurv/shape.h>
#include <libcurv/viewed_shape.h>

namespace curv {

const char glsl_header[] =
   "float fhash(float f)\n"
   "{\n"
   "    uint u = floatBitsToUint(f);\n"
   "    u = ((u & 0x007FFFFFu)\n"
   "        | 0x3F800000u)\n"
   "        ^ (u >> 9);\n"
   "    float f2 = uintBitsToFloat(u);\n"
   "    return fract(f2);\n"
   "}\n";

void glsl_function_export(const Shape_Program& shape, std::ostream& out)
{
    GL_Compiler gl(out, GL_Target::glsl, shape.system());
    At_Program cx(shape);

    out << glsl_header;
    if (shape.viewed_shape_) {
        // output uniform variables for parametric shape
        for (auto& p : shape.viewed_shape_->param_) {
            out << "uniform " << p.second.pconfig_.gltype_ << " "
                << p.second.identifier_ << ";\n";
        }
    }
    gl.define_function("dist", GL_Type::Vec(4), GL_Type::Num(),
        shape.dist_fun_, cx);
    gl.define_function("colour", GL_Type::Vec(4), GL_Type::Vec(3),
        shape.colour_fun_, cx);
}

} // namespace
