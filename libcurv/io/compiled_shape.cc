// Copyright 2016-2021 Doug Moen
// Licensed under the Apache Licence, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/io/compiled_shape.h>

#include <libcurv/context.h>
#include <libcurv/function.h>
#include <libcurv/system.h>

namespace curv { namespace io {

Compiled_Shape::Compiled_Shape(Shape_Program& rshape)
:
    cpp_{rshape.sstate_}
{
    is_2d_ = rshape.is_2d_;
    is_3d_ = rshape.is_3d_;
    bbox_ = rshape.bbox_;

    At_SState cx{rshape.sstate_};

    cpp_.define_function("dist", SC_Type::Num(4), SC_Type::Num(),
        rshape.dist_fun_, cx);
    cpp_.define_function("colour", SC_Type::Num(4), SC_Type::Num(3),
        rshape.colour_fun_, cx);
    cpp_.compile(cx);
    dist_ = (Cpp_Dist_Func) cpp_.get_function("dist");
    colour_ = (Cpp_Colour_Func) cpp_.get_function("colour");
}

void
export_cpp(Shape_Program& shape, std::ostream& out)
{
    SC_Compiler sc(SC_Target::cpp, shape.sstate_);
    At_Program cx(shape);

    out << Cpp_Program::standard_header;
    sc.define_function("dist", SC_Type::Num(4), SC_Type::Num(),
        shape.dist_fun_, cx);
    sc.define_function("colour", SC_Type::Num(4), SC_Type::Num(3),
        shape.colour_fun_, cx);
    sc.emit_objects(out);
}

}} // namespace
