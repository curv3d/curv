// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <iostream>
#include <climits>
#include <cmath>
#include <cstdlib>
#include <chrono>
#include <thread>
#include <glm/geometric.hpp>

#include "export.h"
#include "mesher.h"
#include <libcurv/io/compiled_shape.h>
#include <libcurv/io/mesh.h>
#include <libcurv/shape.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>
#include <libcurv/die.h>

using namespace curv::io;

void export_mesh(Mesh_Format, curv::Value value,
    curv::Program&,
    const Export_Params& params,
    std::ostream& out);

void export_stl(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    Output_File& ofile)
{
    ofile.open();
    export_mesh(Mesh_Format::stl, value, prog, params, ofile.ostream());
}

void export_obj(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    Output_File& ofile)
{
    ofile.open();
    export_mesh(Mesh_Format::obj, value, prog, params, ofile.ostream());
}

void export_x3d(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    Output_File& ofile)
{
    ofile.open();
    export_mesh(Mesh_Format::x3d, value, prog, params, ofile.ostream());
}

void export_gltf(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    Output_File& ofile)
{
    ofile.open();
    export_mesh(Mesh_Format::gltf, value, prog, params, ofile.ostream());
}

void describe_mesh_opts(std::ostream& out)
{
    out <<
    "-O mgen=#smooth|#sharp : Mesh generator algorithm (default #smooth).\n"
    "-O jit : Fast evaluation using JIT compiler (uses C++ compiler).\n"
    "-O vsize=<voxel size>\n"
    "-O vcount=<approximate voxel count>\n"
    "-O eps=<small number> : epsilon to compute normal by partial differences\n"
    "-O adaptive=<0...1> : Deprecated. Use meshlab to simplify mesh.\n"
    ;
}
void describe_colour_mesh_opts(std::ostream& out)
{
    describe_mesh_opts(out);
    out <<
    "-O colouring=#face|#vertex (default #face)\n"
    ;
}

void export_mesh(Mesh_Format format, curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    std::ostream& out)
{
    curv::Shape_Program shape(prog);
    curv::At_Program cx(prog);
    if (!shape.recognize(value, nullptr) || !shape.is_3d_)
        throw curv::Exception(cx, "mesh export: not a 3D shape");

    Mesh_Export opts;
    for (auto& i : params.map_) {
        Param p{params, i};
        if (p.name_ == "mgen") {
            auto val = p.to_symbol();
            if (val == "smooth")
                opts.mgen_ = Mesh_Gen::smooth;
            else if (val == "sharp")
                opts.mgen_ = Mesh_Gen::sharp;
            else if (val == "iso")
                opts.mgen_ = Mesh_Gen::iso;
            else if (val == "hybrid")
                opts.mgen_ = Mesh_Gen::hybrid;
            else if (val == "tmc")
                opts.mgen_ = Mesh_Gen::tmc;
            else
                throw curv::Exception(p,
                    "'mgen' must be #smooth|#sharp|#iso|#hybrid|#tmc");
        } else if (p.name_ == "jit") {
            opts.jit_ = p.to_bool();
        } else if (p.name_ == "vsize") {
            opts.vsize_ = p.to_double();
            if (opts.vsize_ <= 0.0) {
                throw curv::Exception(p, "'vsize' must be positive");
            }
            opts.vcount_ = 0;
        } else if (p.name_ == "vcount") {
            opts.vcount_ = p.to_int(1, INT_MAX);
            opts.vsize_ = 0.0;
        } else if (p.name_ == "eps") {
            opts.eps_ = p.to_double();
        } else if (p.name_ == "adaptive") {
            opts.adaptive_ = p.to_double(1.0);
            if (opts.adaptive_ < 0.0 || opts.adaptive_ > 1.0) {
                throw curv::Exception(p, "'adaptive' must be in range 0...1");
            }
        } else if (format == Mesh_Format::x3d && p.name_ == "colouring") {
            auto val = p.to_symbol();
            if (val == "face")
                opts.colouring_ = Mesh_Export::face_colour;
            else if (val == "vertex")
                opts.colouring_ = Mesh_Export::vertex_colour;
            else {
                throw curv::Exception(p, "'colouring' must be #face or #vertex");
            }
        } else
            p.unknown_parameter();
    }

    std::unique_ptr<curv::io::Compiled_Shape> cshape = nullptr;
    if (opts.jit_) {
        auto cstart_time = std::chrono::steady_clock::now();
        cshape = std::make_unique<curv::io::Compiled_Shape>(shape);
        auto cend_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> compile_time = cend_time - cstart_time;
        std::cerr
            << "Compiled shape in " << compile_time.count() << "s\n";
        std::cerr.flush();
    } else {
        std::cerr <<
            "You are in SLOW MODE. Use '-O jit' to speed up rendering.\n";
    }
    const curv::Shape* pshape;
    if (cshape) pshape = &*cshape; else pshape = &shape;
    bool multithreaded = (cshape != nullptr);

#if LEAN_BUILD
    tmc_mesher(*pshape, multithreaded, opts, cx, format, out);
#else
    switch (opts.mgen_) {
    case Mesh_Gen::smooth:
        vdb_mesher(*pshape, multithreaded, opts, cx, format, out);
        break;
    case Mesh_Gen::sharp:
    case Mesh_Gen::iso:
    case Mesh_Gen::hybrid:
        libfive_mesher(*pshape, multithreaded, opts, cx, format, out);
        break;
    case Mesh_Gen::tmc:
        tmc_mesher(*pshape, multithreaded, opts, cx, format, out);
        break;
    default:
        throw curv::Exception(cx, "mesh export: unknown mesh generator");
    }
#endif
}
