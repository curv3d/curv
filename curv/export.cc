// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include "export.h"

#include <libcurv/geom/viewer/viewer.h>

#include <libcurv/geom/compiled_shape.h>
#include <libcurv/geom/frag.h>
#include <libcurv/geom/png.h>
#include <libcurv/geom/shape.h>
#include <libcurv/geom/viewed_shape.h>

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/filesystem.h>
#include <libcurv/format.h>
#include <libcurv/program.h>
#include <libcurv/range.h>
#include <libcurv/source.h>

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
  out
  << prefix <<
  "-O aa=<supersampling factor for antialiasing> (1 means disabled)\n"
  << prefix <<
  "-O taa=<supersampling factor for temporal antialiasing> (1 means disabled)\n"
  << prefix <<
  "-O fdur=<frame duration, in seconds> : Used with -Otaa and -Oanimate\n"
  << prefix <<
  "-O bg=<background colour>\n"
  ;
}
void describe_frag_opts(std::ostream& out)
{
    describe_frag_options(out, "");
}

bool parse_frag_opt(
    const Export_Params& params,
    const Export_Params::Map::value_type& p,
    geom::Frag_Export& opts)
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
    return false;
}

void export_frag(Value value,
    Program& prog,
    const Export_Params& params,
    Output_File& ofile)
{
    geom::Frag_Export opts;
    for (auto& p : params.map_) {
        if (!parse_frag_opt(params, p, opts))
            params.unknown_parameter(p);
    }

    geom::Shape_Program shape(prog);
    if (!shape.recognize(value)) {
        At_Program cx(prog);
        throw Exception(cx, "not a shape");
    }
    ofile.open();
    geom::export_frag(shape, opts, ofile.ostream());
}

void export_cpp(Value value,
    Program& prog,
    const Export_Params& params,
    Output_File& ofile)
{
    for (auto& p : params.map_) {
        params.unknown_parameter(p);
    }
    geom::Shape_Program shape(prog);
    if (!shape.recognize(value)) {
        At_Program cx(prog);
        throw Exception(cx, "not a shape");
    }
    ofile.open();
    geom::export_cpp(shape, ofile.ostream());
}

bool is_json_data(Value val)
{
    if (val.is_ref()) {
        auto& ref = val.get_ref_unsafe();
        switch (ref.type_) {
        case Ref_Value::ty_string:
        case Ref_Value::ty_list:
        case Ref_Value::ty_record:
            return true;
        default:
            return false;
        }
    } else {
        return true; // null, bool or num
    }
}
void export_json_string(const char* str, std::ostream& out)
{
    out << '"';
    for (const char* p = str; *p != '\0'; ++p) {
        if (*p == '\\' || *p == '"')
            out << '\\';
        out << *p;
    }
    out << '"';
}
bool export_json_value(Value val, std::ostream& out, const Context& cx)
{
    if (val.is_null()) {
        out << "null";
        return true;
    }
    if (val.is_bool()) {
        out << val;
        return true;
    }
    if (val.is_num()) {
        out << dfmt(val.get_num_unsafe(), dfmt::JSON);
        return true;
    }
    assert(val.is_ref());
    auto& ref = val.get_ref_unsafe();
    switch (ref.type_) {
    case Ref_Value::ty_string:
      {
        auto& str = (String&)ref;
        export_json_string(str.c_str(), out);
        return true;
      }
    case Ref_Value::ty_list:
      {
        auto& list = (List&)ref;
        out << "[";
        bool first = true;
        size_t i = 0;
        for (auto e : list) {
            if (is_json_data(e)) {
                if (!first) out << ",";
                first = false;
                export_json_value(e, out, At_Index(i, cx));
            }
            ++i;
        }
        out << "]";
        return true;
      }
    case Ref_Value::ty_record:
      {
        auto& record = (Record&)ref;
        out << "{";
        bool first = true;
        record.each_field(cx, [&](Symbol id, Value val) {
            if (is_json_data(val)) {
                if (!first) out << ",";
                first = false;
                export_json_string(id.c_str(), out);
                out << ":";
                export_json_value(val, out, At_Field(id.c_str(), cx));
            }
        });
        out << "}";
        return true;
      }
    default:
        return false;
    }
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
    At_Program cx(prog);
    if (export_json_value(value, ofile.ostream(), cx))
        ofile.ostream() << "\n";
    else {
        throw Exception(cx, "value can't be converted to JSON");
    }
}

void export_json_api(Value value,
    Program& prog, const Export_Params& params, Output_File& ofile)
{
    geom::Frag_Export opts;
    for (auto& p : params.map_) {
        if (!parse_frag_opt(params, p, opts))
            params.unknown_parameter(p);
    }

    ofile.open();
    At_Program cx(prog);
    geom::Shape_Program shape(prog);
    if (!shape.recognize(value)) {
        ofile.ostream() << "{\"value\":";
        if (!export_json_value(value, ofile.ostream(), cx))
            ofile.ostream() << "{\"<not-a-value>\":true}";
        ofile.ostream() << "}\n";
    } else {
        geom::Viewed_Shape vshape(shape, opts);
        ofile.ostream() << "{\"shape\":{"
            << "\"is_2d\":" << Value{shape.is_2d_}
            << ",\"is_3d\":" << Value{shape.is_3d_}
            << ",\"bbox\":[[" << dfmt(shape.bbox_.xmin, dfmt::JSON)
                << "," << dfmt(shape.bbox_.ymin, dfmt::JSON)
                << "," << dfmt(shape.bbox_.zmin, dfmt::JSON)
                << "],[" << dfmt(shape.bbox_.xmax, dfmt::JSON)
                << "," << dfmt(shape.bbox_.ymax, dfmt::JSON)
                << "," << dfmt(shape.bbox_.zmax, dfmt::JSON)
            << "]],\"shader\":";
        export_json_string(vshape.frag_.c_str(), ofile.ostream());
        ofile.ostream() << "}}\n";
    }
}

// wrapper that exports image sequences if requested
void export_all_png(
    const geom::Shape_Program& shape,
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

    geom::Shape_Program shape(prog);
    At_Program cx(prog);
    if (!shape.recognize(value))
        throw Exception(cx, "not a shape");
    if (shape.is_2d_) {
        if (shape.bbox_.infinite2())
            throw Exception(cx, "can't export an infinite 2D shape to PNG");
        if (shape.bbox_.empty2())
            throw Exception(cx, "can't export an empty 2D shape to PNG");
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
    {"frag", {export_frag,
              "GLSL fragment shader (shape only, shadertoy.com compatible)",
              describe_frag_opts}},
    {"json", {export_json, "JSON expression", describe_no_opts}},
    {"json-api", {export_json_api,
        "program execution results as a JSON stream", describe_frag_opts}},
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
