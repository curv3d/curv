// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/glsl.h>

#include <libcurv/context.h>
#include <libcurv/function.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/shape.h>
#include <libcurv/viewed_shape.h>

namespace curv {

void glsl_function_export(const Shape_Program& shape, std::ostream& out)
{
    SC_Compiler sc(SC_Target::glsl, shape.sstate_);
    At_Program cx(shape);

    if (shape.viewed_shape_) {
        // output uniform variables for parametric shape
        for (auto& p : shape.viewed_shape_->param_) {
            out << "uniform " << p.second.pconfig_.sctype_ << " "
                << p.second.identifier_ << ";\n";
        }
    }
    sc.define_function("dist", SC_Type::Num(4), SC_Type::Num(),
        shape.dist_fun_, cx);
    sc.define_function("colour", SC_Type::Num(4), SC_Type::Num(3),
        shape.colour_fun_, cx);
    sc.emit_objects(out);
}

} // namespace
