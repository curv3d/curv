// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/gpu_program.h>

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/format.h>
#include <libcurv/program.h>

namespace curv {

GPU_Program::GPU_Program(Program& prog)
:
    system_(prog.system()),
    nub_(nub_phrase(prog.phrase_))
{
    // mark initial state (no shape has been recognized yet)
    is_2d_ = false;
    is_3d_ = false;
}

Location
GPU_Program::location() const
{
    return nub_->location();
}

bool
GPU_Program::recognize(Value val, const Frag_Export& opts)
{
    if (location().source().type_ == Source::Type::gpu) {
        // Note: throw exception if val is not a GPU program.
        static Symbol_Ref is_2d_key = "is_2d";
        static Symbol_Ref is_3d_key = "is_3d";
        static Symbol_Ref bbox_key = "bbox";
        static Symbol_Ref shader_key = "shader";
        static Symbol_Ref parameters_key = "parameters";
        static Symbol_Ref name_key = "name";
        static Symbol_Ref value_key = "value";
        static Symbol_Ref label_key = "label";
        static Symbol_Ref config_key = "config";

        At_Program cx(*this);
        auto r = val.to<Record>(cx);

        is_2d_ = r->getfield(is_2d_key, cx).to_bool(At_Field("is_2d", cx));
        is_3d_ = r->getfield(is_3d_key, cx).to_bool(At_Field("is_3d", cx));
        if (!is_2d_ && !is_3d_)
            throw Exception(cx,
                "at least one of is_2d and is_3d must be true");

        bbox_ = BBox::from_value(
            r->getfield(bbox_key, cx),
            At_Field("bbox", cx));

        vshape_.frag_ =
            r->getfield(shader_key, cx)
            .to<String>(At_Field("shader",cx))->c_str();

        At_Field pcx("parameters",cx);
        auto parameters = r->getfield(parameters_key, cx).to<List>(pcx);
        At_Index picx(0, pcx);
        for (auto p : *parameters) {
            auto prec = p.to<Record>(picx);
            auto name = prec->getfield(name_key, picx)
                .to<String>(At_Field("name",picx))->c_str();
            auto label = prec->getfield(label_key, picx)
                .to<String>(At_Field("label",picx))->c_str();
            Picker::Config config(
                prec->getfield(config_key, picx),
                At_Field("config", picx));
            auto state_val = prec->getfield(value_key, picx);
            Picker::State state(config.type_, state_val, At_Field("value",picx));
            vshape_.param_.insert(
                std::pair<const std::string,Viewed_Shape::Parameter>{
                    label,
                    Viewed_Shape::Parameter{name, config, state}});
            ++picx.index_;
        }

        return true;
    }
    Shape_Program shape(system_, nub_);
    if (!shape.recognize(val))
        return false;
    is_2d_ = shape.is_2d_;
    is_3d_ = shape.is_3d_;
    bbox_ =  shape.bbox_;
    Viewed_Shape vshape(shape, opts);
    std::swap(vshape_, vshape);
    return true;
}

void
GPU_Program::write_json(std::ostream& out) const
{
    out << "{"
        << "\"is_2d\":" << Value{is_2d_}
        << ",\"is_3d\":" << Value{is_3d_}
        << ",\"bbox\":[[" << dfmt(bbox_.xmin, dfmt::JSON)
            << "," << dfmt(bbox_.ymin, dfmt::JSON)
            << "," << dfmt(bbox_.zmin, dfmt::JSON)
            << "],[" << dfmt(bbox_.xmax, dfmt::JSON)
            << "," << dfmt(bbox_.ymax, dfmt::JSON)
            << "," << dfmt(bbox_.zmax, dfmt::JSON)
        << "]],";
    vshape_.write_json(out);
    out << "}";
}

void
GPU_Program::write_curv(std::ostream& out) const
{
    out << "{\n"
        << "  is_2d: " << Value{is_2d_} << ";\n"
        << "  is_3d: " << Value{is_3d_} << ";\n"
        << "  bbox: [[" << dfmt(bbox_.xmin, dfmt::JSON)
            << "," << dfmt(bbox_.ymin, dfmt::JSON)
            << "," << dfmt(bbox_.zmin, dfmt::JSON)
            << "],[" << dfmt(bbox_.xmax, dfmt::JSON)
            << "," << dfmt(bbox_.ymax, dfmt::JSON)
            << "," << dfmt(bbox_.zmax, dfmt::JSON)
        << "]];\n";
    vshape_.write_curv(out);
    out << "}\n";
}

} // namespace
