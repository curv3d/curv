// Copyright 2016-2018 Doug Moen
// Licensed under the Apache Licence, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/compiled_shape.h>

#include <libcurv/geom/cpp_program.h>

#include <libcurv/context.h>
#include <libcurv/function.h>
#include <libcurv/system.h>

namespace curv { namespace geom {

Compiled_Shape::Compiled_Shape(Shape_Program& rshape)
{
    is_2d_ = rshape.is_2d_;
    is_3d_ = rshape.is_3d_;
    bbox_ = rshape.bbox_;

    At_System cx{rshape.system_};
    CPP_Program cpp{rshape.system_};

    cpp.define_function("dist", GL_Type::Vec(4), GL_Type::Num(),
        rshape.dist_fun_, cx);
    cpp.define_function("colour", GL_Type::Vec(4), GL_Type::Vec(3),
        rshape.colour_fun_, cx);
    cpp.compile();
    dist_ = (void (*)(const glm::vec4*,float*))cpp.get_function("dist");
    colour_ = (void (*)(const glm::vec4*,glm::vec3*))cpp.get_function("colour");
}

void
export_cpp(Shape_Program& shape, std::ostream& out)
{
    GL_Compiler gl(out, GL_Target::cpp, shape.system());
    At_Program cx(shape);

    out << CPP_Program::standard_header;
    gl.define_function("dist", GL_Type::Vec(4), GL_Type::Num(),
        shape.dist_fun_, cx);
    gl.define_function("colour", GL_Type::Vec(4), GL_Type::Vec(3),
        shape.colour_fun_, cx);
}

}} // namespace
