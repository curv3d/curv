// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/gpu_program.h>

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
