// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/json.h>
#include <libcurv/viewed_shape.h>
#include <iostream>
#include <cctype>

namespace curv {

Viewed_Shape::Viewed_Shape(const Shape_Program& shape, const Frag_Export& opts)
{
    // Recognize a parametric shape S,
    // which contains constructor & argument fields.
    // If found:
    // * Extract the parameters.
    //   Each parameter has: name, value, optional picker.
    //   The parameters are stored in the Viewed_Shape.
    // * Create a parameter record A (a Value). Parameters that have a picker
    //   get a Reactive Value as their argument. Non-picker parameters get a
    //   proper value taken from S.parameter.
    // * Evaluate S.call(A).
    // * The result must be a shape record S1 (otherwise error).
    // * compile S1.dist and S1.colour into GLSL. References to reactive
    //   values are converted to uniform variable references. Note that S1.bbox
    //   etc are ignored. (glsl_function_export)
    // If not found:
    // * compile S.dist and S.colour into GLSL. (glsl_function_export)

    // Recognize a parametric shape, create ptable.
    // If found, create S1. Store ptable ptr in S1.
    //   Call export_frag(S1), which calls glsl_function_export(S1),
    //   which outputs uniform variable references from ptable.
    // Else, call export_frag(S), the boring case.

    // Viewed_Shape data structures:
    // * A set of reactive values, used by the SC compiler. These are not
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

    static Symbol_Ref argument_key = make_symbol("argument");
    static Symbol_Ref constructor_key = make_symbol("constructor");
    static Symbol_Ref picker_key = make_symbol("picker");

    // Recognize a parametric shape (it has `argument` and `constructor` fields).
    At_System cx{shape.system_};
    Shared<Record> sh_argument = nullptr;
    if (shape.record_->hasfield(argument_key)) {
        sh_argument = shape.record_->getfield(argument_key, cx).to<Record>(cx);
    }
    Shared<Closure> sh_constructor = nullptr;
    if (sh_argument && shape.record_->hasfield(constructor_key)) {
        sh_constructor = shape.record_->getfield(constructor_key, cx).to<Closure>(cx);
    }
    if (sh_argument && sh_constructor) {
        // We have a parametric shape.
        auto cparams = make<DRecord>();
        record_pattern_each_parameter(*sh_constructor, shape.system_,
            [&](Symbol_Ref name, Value pred, Value value) -> void {
                auto pred_record = pred.dycast<Record>();
                if (pred_record && pred_record->hasfield(picker_key)) {
                    auto picker = pred_record->getfield(picker_key,cx);
                    std::string id{"rv_"};
                    for (const char*p = name.c_str(); *p; ++p) {
                        if (std::isalnum(*p))
                            id += *p;
                        else
                            id += '_';
                    }
                    Picker::Config config(picker, cx);
                    Picker::State state{
                        config.type_,
                        value,
                        At_System{shape.system_}};
                    param_.insert(std::pair<const std::string,Parameter>{
                        name.c_str(),
                        Parameter{id, config, state}});
                    cparams->fields_[name] =
                        {make<Uniform_Variable>(name, id, config.sctype_)};
                } else {
                    cparams->fields_[name] = value;
                }
            });
        std::unique_ptr<Frame> f2 {
            Frame::make(sh_constructor->nslots_, shape.system_, nullptr, nullptr, nullptr)
        };
        Value result = sh_constructor->call({cparams}, *f2);
        //std::cerr << "parametric shape: " << result << "\n";

        auto r = result.dycast<Record>();
        if (r == nullptr)
            throw Exception{cx, stringify(
                "bad parametric shape: call function returns non-record: ",
                result)};
        Shape_Program shape2(shape, r, this);

        std::stringstream frag;
        export_frag(shape2, opts, frag);
        frag_ = frag.str();
    } else {
        // Non-parametric case.
        std::stringstream frag;
        export_frag(shape, opts, frag);
        frag_ = frag.str();
    }
}

void
Viewed_Shape::write_json(std::ostream& out) const
{
    out << "\"shader\":";
    write_json_string(frag_.c_str(), out);
    out << ",\"parameters\":[";
    bool first = true;
    for (auto& p : param_) {
        if (!first) out << ",";
        first = false;
        out << "{";
        out << "\"name\":\"rv_" << p.first << "\"";
        out << ",\"type\":\"" << p.second.pconfig_.sctype_ << "\"";
        out << ",\"value\":";
        p.second.pstate_.write_json(out, p.second.pconfig_.type_);
        out << ",\"label\":"; write_json_string(p.first.c_str(), out);
        out << ",\"config\":"; p.second.pconfig_.write_json(out);
        out << "}";
    }
    out << "]";
}

void
Viewed_Shape::write_curv(std::ostream& out) const
{
    out << "  shader:\n    ";
    write_curv_string(frag_.c_str(), 4, out);
    out << ";\n  parameters: [\n";
    for (auto& p : param_) {
        out << "    {";
        out << "name: \"rv_" << p.first << "\"";
        out << ", type: \"" << p.second.pconfig_.sctype_ << "\"";
        out << ", value: ";
        p.second.pstate_.write_curv(out, p.second.pconfig_.type_);
        out << ", label: "; write_curv_string(p.first.c_str(), 0, out);
        out << ", config: "; p.second.pconfig_.write_curv(out);
        out << "};\n";
    }
    out << "  ];\n";
}

} // namespace
