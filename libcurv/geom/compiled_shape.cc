// Copyright 2016-2018 Doug Moen
// Licensed under the Apache Licence, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/compiled_shape.h>

#include <libcurv/context.h>
#include <libcurv/function.h>
#include <libcurv/system.h>

namespace curv { namespace geom {

Compiled_Shape::Compiled_Shape(Shape_Program& rshape)
:
    cpp_{rshape.system_}
{
    is_2d_ = rshape.is_2d_;
    is_3d_ = rshape.is_3d_;
    bbox_ = rshape.bbox_;

    At_System cx{rshape.system_};

    cpp_.define_function("dist", GL_Type::Vec(4), GL_Type::Num(),
        rshape.dist_fun_, cx);
    cpp_.define_function("colour", GL_Type::Vec(4), GL_Type::Vec(3),
        rshape.colour_fun_, cx);
    cpp_.compile();
    dist_ = (Cpp_Dist_Func) cpp_.get_function("dist");
    colour_ = (Cpp_Colour_Func) cpp_.get_function("colour");
}

void
export_cpp(Shape_Program& shape, std::ostream& out)
{
    GL_Compiler gl(out, GL_Target::cpp, shape.system());
    At_Program cx(shape);

    out << Cpp_Program::standard_header;
    gl.define_function("dist", GL_Type::Vec(4), GL_Type::Num(),
        shape.dist_fun_, cx);
    gl.define_function("colour", GL_Type::Vec(4), GL_Type::Vec(3),
        shape.colour_fun_, cx);
}

}} // namespace
