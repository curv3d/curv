// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/context.h>
#include <libcurv/geom/viewed_shape.h>
#include <iostream>

namespace curv {
namespace geom {

Viewed_Shape::Viewed_Shape(const Shape_Program& shape, const Frag_Export& opts)
{
    // Recognize a parametric shape S, which contains call & parameter fields.
    // If found:
    // * Extract the parameters.
    //   Each parameter has: name, value, optional picker.
    //   The parameters are stored in the Viewed_Shape.
    // * Create a parameter record A (a Value). Parameters that have a picker
    //   get a Reactive Value as their argument. Non-picker parameters get a
    //   proper value taken from S.parameter.
    // * Evaluate S.call(A).
    // * The result must be a shape record S1 (otherwise error).
    // * GL compile S1.dist and S1.colour into GLSL. References to reactive
    //   values are converted to uniform variable references. Note that S1.bbox
    //   etc are ignored. (glsl_function_export)
    // If not found:
    // * GL compile S.dist and S.colour into GLSL. (glsl_function_export)

    // Recognize a parametric shape, create ptable.
    // If found, create S1. Store ptable ptr in S1.
    //   Call export_frag(S1), which calls glsl_function_export(S1),
    //   which outputs uniform variable references from ptable.
    // Else, call export_frag(S), the boring case.

    // Viewed_Shape data structures:
    // * A set of reactive values, used by the GL compiler. These are not
    //   stored in the Viewed_Shape itself, because it is self contained,
    //   doesn't reference objects shared with other threads.
    // * A parameter table, used by the Viewer in the OpenGL main loop.
    //   This is stored in the Viewed_Shape.
    //   It's an array, not a map. Each element:
    //   * GLSL variable name, referenced by GLSL frag shader, passed to
    //     glGetUniformLocation to get GLint id used to set the value.
    //   * picker config
    //     * type, an enum I guess
    //     * optional config values, like (lo,hi) for a slider
    //   * current picker value, value type depends on picker type
    //   * A picker descriptor is either a variant record (a struct with an
    //     enum type and a union of type dependent values), or it is a
    //     unique_ptr to a picker subclass instance, and RTTI is used.
    //   If I use IMGUI, then I iterate over the parameter table and render
    //   each picker.

    // Recognize a parametric shape (it has `parameter` and `call` fields).
    At_System cx{shape.system_};
    Shared<Record> sh_parameter = nullptr;
    if (shape.record_->hasfield("parameter")) {
        sh_parameter = shape.record_->getfield("parameter", cx).to<Record>(cx);
    }
    Shared<Closure> sh_call = nullptr;
    if (sh_parameter && shape.record_->hasfield("call")) {
        sh_call = shape.record_->getfield("call", cx).to<Closure>(cx);
    }
    if (sh_parameter && sh_call) {
        // We have a parametric shape.
        auto cparams = make<DRecord>();
        record_pattern_each_parameter(*sh_call, shape.system_,
            [&](Symbol name, Value pred, Value value) -> void {
                auto picker = pred.dycast<Picker>();
                std::cerr << "> " << name;
                if (picker) {
                    std::cerr << " :: ";
                    picker->config_.write(std::cerr);
                }
                std::cerr << " = " << value << "\n";
                if (picker) {
                    params_.push_back(Parameter{name.c_str(), picker->config_,
                        Picker::State{
                            picker->config_.type_,
                            value,
                            At_System{shape.system_}}});
                    cparams->fields_[name] =
                        {make<Uniform_Variable>(name, picker->config_.type_)};
                } else {
                    cparams->fields_[name] = value;
                }
            });
        std::unique_ptr<Frame> f2 {
            Frame::make(sh_call->nslots_, shape.system_, nullptr, nullptr, nullptr)
        };
        Value result = sh_call->call({cparams}, *f2);
        std::cerr << "parametric shape: " << result << "\n";
    }

    // Non-parametric case.
    std::stringstream frag;
    export_frag(shape, opts, frag);
    frag_ = frag.str();
}

}} // namespace
