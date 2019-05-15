// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include "export.h"

#include <libcurv/geom/compiled_shape.h>
#include <libcurv/geom/png.h>
#include <libcurv/geom/viewer/viewer.h>

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/filesystem.h>
#include <libcurv/format.h>
#include <libcurv/frag.h>
#include <libcurv/gpu_program.h>
#include <libcurv/json.h>
#include <libcurv/program.h>
#include <libcurv/range.h>
#include <libcurv/shape.h>
#include <libcurv/source.h>
#include <libcurv/viewed_shape.h>

#include <glm/vec2.hpp>

#include <algorithm>
#include <climits>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>

using namespace curv;

Param_Program::Param_Program(
    const Export_Params& params,
    const Export_Params::Map::value_type& p)
:
    Program(
        make<String_Source>("", stringify("-O ",p.first,"=",p.second)),
        params.system_,
        Program_Opts().skip_prefix(4+p.first.size()))
{}

Value Param_Program::eval()
{
    if (phrase_ == nullptr)
        compile();
    if (isa<const Empty_Phrase>(nub_phrase(phrase_)))
        throw Exception(At_Program(*this), "missing argument");
    return Program::eval();
}

void Export_Params::unknown_parameter(const Map::value_type& p) const
{
    auto src = make<String_Source>("", stringify("-O ",p.first,"=",p.second));
    Location loc{*src, {3, unsigned(3+p.first.size())}};
    if (format_.empty()) {
        throw Exception(At_Token{loc, system_}, stringify(
            "'",p.first,"': Unknown -O parameter.\n"
            "Use 'curv --help' for help."));
    } else {
        throw Exception(At_Token{loc, system_}, stringify(
            "'",p.first,"': "
            "Unknown -O parameter for output format '",format_,"'.\n"
            "Use 'curv --help -o ",format_,"' for help."));
    }
}

int Export_Params::to_int(const Map::value_type& p, int lo, int hi) const
{
    Param_Program pp(*this, p);
    return pp.eval().to_int(lo, hi, At_Program(pp));
}

double Export_Params::to_double(const Map::value_type& p) const
{
    Param_Program pp(*this, p);
    return pp.eval().to_num(At_Program(pp));
}

glm::dvec3 Export_Params::to_vec3(const Map::value_type& p) const
{
    Param_Program pp(*this, p);
    At_Program cx(pp);
    glm::dvec3 result;
    Value val = pp.eval();
    auto list = val.to<List>(cx);
    list->assert_size(3, cx);
    result.x = list->at(0).to_num(At_Index(0, cx));
    result.y = list->at(1).to_num(At_Index(1, cx));
    result.z = list->at(2).to_num(At_Index(2, cx));
    return result;
}

bool Export_Params::to_bool(const Map::value_type& p) const
{
    if (p.second.empty())
        return true;
    Param_Program pp(*this, p);
    return pp.eval().to_bool(At_Program(pp));
}

curv::Symbol_Ref Export_Params::to_symbol(const Map::value_type& p) const
{
    Param_Program pp(*this, p);
    return pp.eval().to<Symbol>(At_Program(pp));
}

void export_curv(Value value,
    Program&,
    const Export_Params& params,
    Output_File& ofile)
{
    for (auto& p : params.map_) {
        params.unknown_parameter(p);
    }
    ofile.open();
    ofile.ostream() << value << "\n";
}

void describe_frag_options(std::ostream& out, const char*prefix)
{
  Frag_Export opts;
  out
  << prefix <<
  "-O aa=<supersampling factor for antialiasing> (1 means disabled)\n"
  << prefix <<
  "-O taa=<supersampling factor for temporal antialiasing> (1 means disabled)\n"
  << prefix <<
  "-O fdur=<frame duration, in seconds> : Used with -Otaa and -Oanimate\n"
  << prefix <<
  "-O bg=<background colour>\n"
  << prefix <<
  "-O ray_max_iter=<maximum # of ray-march iterations> (default "
    << opts.ray_max_iter_ << ")\n"
  << prefix <<
  "-O ray_max_depth=<maximum ray-marching depth> (default "
    << opts.ray_max_depth_ << ")\n"
  ;
}
void describe_frag_opts(std::ostream& out)
{
    describe_frag_options(out, "");
}

bool parse_frag_opt(
    const Export_Params& params,
    const Export_Params::Map::value_type& p,
    Frag_Export& opts)
{
    if (p.first == "aa") {
        opts.aa_ = params.to_int(p, 1, INT_MAX);
        return true;
    }
    if (p.first == "taa") {
        opts.taa_ = params.to_int(p, 1, INT_MAX);
        return true;
    }
    if (p.first == "fdur") {
        opts.fdur_ = params.to_double(p);
        return true;
    }
    if (p.first == "bg") {
        opts.bg_ = params.to_vec3(p);
        return true;
    }
    if (p.first == "ray_max_iter") {
        opts.ray_max_iter_ = params.to_int(p, 1, INT_MAX);
        return true;
    }
    if (p.first == "ray_max_depth") {
        opts.ray_max_depth_ = params.to_double(p);
        return true;
    }
    return false;
}

void export_cpp(Value value,
    Program& prog,
    const Export_Params& params,
    Output_File& ofile)
{
    for (auto& p : params.map_) {
        params.unknown_parameter(p);
    }
    Shape_Program shape(prog);
    if (!shape.recognize(value)) {
        At_Program cx(prog);
        throw Exception(cx, "not a shape");
    }
    ofile.open();
    geom::export_cpp(shape, ofile.ostream());
}

void export_json(Value value,
    Program& prog,
    const Export_Params& params,
    Output_File& ofile)
{
    for (auto& p : params.map_) {
        params.unknown_parameter(p);
    }
    ofile.open();
    write_json_value(value, ofile.ostream());
    ofile.ostream() << "\n";
}

void export_gpu(Value value,
    Program& prog, const Export_Params& params, Output_File& ofile)
{
    Frag_Export opts;
    for (auto& p : params.map_) {
        if (!parse_frag_opt(params, p, opts))
            params.unknown_parameter(p);
    }

    ofile.open();
    At_Program cx(prog);
    GPU_Program gprog(prog);
    if (!gprog.recognize(value, opts))
        throw Exception(cx, "not a shape");
    gprog.write_curv(ofile.ostream());
}

// wrapper that exports image sequences if requested
void export_all_png(
    const Shape_Program& shape,
    geom::Image_Export& ix,
    double animate,
    Output_File& ofile)
{
    if (animate <= 0.0) {
        // export single image
        geom::export_png(shape, ix, ofile);
        return;
    }

    const char* ipath = ofile.path_.c_str();
    const char* p = strchr(ipath, '*');
    if (p == nullptr) {
        throw Exception(At_System(shape.system_),
          "'-O animate=' requires pathname in '-o pathname' to contain a '*'");
    }
    Range<const char*> prefix(ipath, p);
    Range<const char*> suffix(p+1, strlen(p+1));

    unsigned count = unsigned(animate / ix.fdur_ + 0.5);
    if (count == 0) count = 1;
    unsigned digs = ndigits(count);
    double fstart = ix.fstart_;
    for (unsigned i = 0; i < count; ++i) {
        ix.fstart_ = fstart + i * ix.fdur_;
        char num[12];
        snprintf(num, sizeof(num), "%0*d", digs, i);
        auto opath = stringify(prefix, num, suffix);
        Output_File oofile{shape.system_};
        oofile.set_path(opath->c_str());
        geom::export_png(shape, ix, oofile);
        oofile.commit();
        //std::cerr << ".";
        //std::cerr.flush();
    }
    //std::cerr << "done\n";
}

void describe_png_opts(std::ostream& out)
{
    out <<
    "-v : verbose output logged to stderr\n"
    "-O xsize=<image width in pixels>\n"
    "-O ysize=<image height in pixels>\n"
    "-O fstart=<animation frame start time, in seconds> (default 0)\n";
    describe_frag_opts(out);
    out <<
    "-O animate=<duration of animation> (exports an image sequence)\n";
}

void export_png(Value value,
    Program& prog,
    const Export_Params& params,
    Output_File& ofile)
{
    geom::Image_Export ix;

    int xsize = 0;
    int ysize = 0;
    double animate = 0.0;
    for (auto& p : params.map_) {
        if (parse_frag_opt(params, p, ix)) {
            ;
        } else if (p.first == "xsize") {
            xsize = params.to_int(p, 1, INT_MAX);
        } else if (p.first == "ysize") {
            ysize = params.to_int(p, 1, INT_MAX);
        } else if (p.first == "fstart") {
            ix.fstart_ = params.to_double(p);
        } else if (p.first == "animate") {
            animate = params.to_double(p);
        } else {
            params.unknown_parameter(p);
        }
    }

    Shape_Program shape(prog);
    At_Program cx(prog);
    if (!shape.recognize(value))
        throw Exception(cx, "not a shape");
    if (shape.is_2d_) {
      #if 0
        if (shape.bbox_.infinite2())
            throw Exception(cx, "can't export an infinite 2D shape to PNG");
        if (shape.bbox_.empty2())
            throw Exception(cx, "can't export an empty 2D shape to PNG");
      #else
        // bug #67
        if (shape.bbox_.infinite2() || shape.bbox_.empty2()) {
            shape.bbox_.xmin = -10.0;
            shape.bbox_.ymin = -10.0;
            shape.bbox_.xmax = +10.0;
            shape.bbox_.ymax = +10.0;
        }
      #endif
        double dx = shape.bbox_.xmax - shape.bbox_.xmin;
        double dy = shape.bbox_.ymax - shape.bbox_.ymin;
        if (!xsize && !ysize) {
            if (dx > dy)
                xsize = 500;
            else
                ysize = 500;
        }
        if (xsize && !ysize) {
            ix.size.x = xsize;
            ix.pixel_size = dx / double(xsize);
            ix.size.y = (int) round(dy / dx * double(xsize));
            if (ix.size.y == 0) ++ix.size.y;
        } else if (!xsize && ysize) {
            ix.size.y = ysize;
            ix.pixel_size = dy / double(ysize);
            ix.size.x = (int) round(dx / dy * double(ysize));
            if (ix.size.x == 0) ++ix.size.x;
        } else {
            ix.size.x = xsize;
            ix.size.y = ysize;
            ix.pixel_size = std::min(dx / double(xsize), dy / double(ysize));
        }
    } else {
        // 3D export to PNG is basically a screenshot.
        // We ignore the bounding box.
        if (!xsize && !ysize)
            ix.size.x = ix.size.y = 500;
        else if (!xsize)
            ix.size.x = ix.size.y = ysize;
        else if (!ysize)
            ix.size.x = ix.size.y = xsize;
        else {
            ix.size.x = xsize;
            ix.size.y = ysize;
        }
    }
    ix.verbose_ = params.verbose_;
    if (params.verbose_) {
        std::cerr << ix.size.x<<"×"<<ix.size.y<<" pixels";
        if (ix.aa_ > 1)
            std::cerr << ", " << ix.aa_<<"× antialiasing";
        if (ix.taa_ > 1)
            std::cerr << ", " << ix.aa_<<"× temporal antialiasing";
        std::cerr << std::endl;
    }
    export_all_png(shape, ix, animate, ofile);
}

void describe_no_opts(std::ostream&) {}

std::map<std::string, Exporter> exporters = {
    {"curv", {export_curv, "Curv expression", describe_no_opts}},
    {"stl", {export_stl, "STL mesh file (3D shape only)", describe_mesh_opts}},
    {"obj", {export_obj, "OBJ mesh file (3D shape only)", describe_mesh_opts}},
    {"x3d", {export_x3d, "X3D colour mesh file (3D shape only)",
             describe_colour_mesh_opts}},
    {"gpu", {export_gpu, "compiled GPU program, in Curv format (shape only)",
        describe_frag_opts}},
    {"json", {export_json, "JSON expression", describe_no_opts}},
    {"cpp", {export_cpp, "C++ source file (shape only)", describe_no_opts}},
    {"png", {export_png, "PNG image file (shape only)", describe_png_opts}},
};

void parse_viewer_config(
    const Export_Params& params,
    geom::viewer::Viewer_Config& opts)
{
    opts.verbose_ = params.verbose_;
    for (auto& p : params.map_) {
        if (p.first == "lazy")
            opts.lazy_ = params.to_bool(p);
        else if (!parse_frag_opt(params, p, opts))
            params.unknown_parameter(p);
    }
}
void describe_viewer_options(std::ostream& out, const char* prefix)
{
    describe_frag_options(out, prefix);
    out
    << prefix <<
    "-O lazy : Redraw only on user input. Disables animation & FPS counter.\n"
    ;
}
