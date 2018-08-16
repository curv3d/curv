// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include "export.h"

#include <libcurv/geom/compiled_shape.h>
#include <libcurv/geom/frag.h>
#include <libcurv/geom/export_png.h>
#include <libcurv/geom/shape.h>

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/filesystem.h>

#include <glm/vec2.hpp>

#include <algorithm>
#include <climits>
#include <cstdlib>
#include <fstream>
#include <sstream>

void Export_Params::unknown_parameter(const Map::value_type& p) const
{
    std::cerr
        << "-O " << p.first << ": unknown parameter name\n"
        << "Use 'curv --help -o " << format << "' for help.\n";
    exit(EXIT_FAILURE);
}

void Export_Params::bad_argument(const Map::value_type& p, const char* msg) const
{
    if (p.second.empty())
        msg = "missing argument";
    std::cerr
        << "-O " << p.first << "=" << p.second << ": " << msg << "\n"
        << "Use 'curv --help -o " << format << "' for help.\n";
    exit(EXIT_FAILURE);
}

int Export_Params::to_int(const Map::value_type& p, int lo, int hi) const
{
    const char* str = p.second.c_str();
    char* endptr;
    long n = strtol(str, &endptr, 10);
    if (*str != '\0' && *endptr == '\0') {
        if (n < lo || n > hi)
            bad_argument(p, "integer value is outsize of legal range");
        return int(n);
    }
    bad_argument(p, "argument is not an integer");
}

double Export_Params::to_double(const Map::value_type& p) const
{
    const char* str = p.second.c_str();
    char *endptr;
    double result = strtod(str, &endptr);
    if (*str == '\0' || *endptr != '\0' || result != result)
        bad_argument(p, "argument is not a number");
    return result;
}

void export_curv(curv::Value value,
    curv::Program&,
    const Export_Params& params,
    curv::Output_File& ofile)
{
    for (auto& p : params.map) {
        params.unknown_parameter(p);
    }
    ofile.open();
    ofile.ostream() << value << "\n";
}

const char export_frag_help[] =
    "-O aa=<supersampling factor for antialiasing; 1 means disabled>\n"
    "-O taa=<supersampling factor for temporal antialiasing; 1 means disabled>\n"
    "-O delay=<frame duration for animation (used with TAA)>\n";

void export_frag(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    curv::Output_File& ofile)
{
    curv::geom::Frag_Export opts;
    for (auto& p : params.map) {
        if (p.first == "aa") {
            opts.aa_ = params.to_int(p, 1, INT_MAX);
        } else if (p.first == "taa") {
            opts.taa_ = params.to_int(p, 1, INT_MAX);
        } else if (p.first == "delay") {
            opts.delay_ = params.to_double(p);
        } else {
            params.unknown_parameter(p);
        }
    }

    curv::geom::Shape_Program shape(prog);
    if (!shape.recognize(value)) {
        curv::At_Program cx(prog);
        throw curv::Exception(cx, "not a shape");
    }
    ofile.open();
    curv::geom::export_frag(shape, opts, ofile.ostream());
}

void export_cpp(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    curv::Output_File& ofile)
{
    for (auto& p : params.map) {
        params.unknown_parameter(p);
    }
    curv::geom::Shape_Program shape(prog);
    if (!shape.recognize(value)) {
        curv::At_Program cx(prog);
        throw curv::Exception(cx, "not a shape");
    }
    ofile.open();
    curv::geom::export_cpp(shape, ofile.ostream());
}

bool is_json_data(curv::Value val)
{
    if (val.is_ref()) {
        auto& ref = val.get_ref_unsafe();
        switch (ref.type_) {
        case curv::Ref_Value::ty_string:
        case curv::Ref_Value::ty_list:
        case curv::Ref_Value::ty_record:
            return true;
        default:
            return false;
        }
    } else {
        return true; // null, bool or num
    }
}
bool export_json_value(curv::Value val, std::ostream& out)
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
        out << curv::dfmt(val.get_num_unsafe(), curv::dfmt::JSON);
        return true;
    }
    assert(val.is_ref());
    auto& ref = val.get_ref_unsafe();
    switch (ref.type_) {
    case curv::Ref_Value::ty_string:
      {
        auto& str = (curv::String&)ref;
        out << '"';
        for (auto c : str) {
            if (c == '\\' || c == '"')
                out << '\\';
            out << c;
        }
        out << '"';
        return true;
      }
    case curv::Ref_Value::ty_list:
      {
        auto& list = (curv::List&)ref;
        out << "[";
        bool first = true;
        for (auto e : list) {
            if (is_json_data(e)) {
                if (!first) out << ",";
                first = false;
                export_json_value(e, out);
            }
        }
        out << "]";
        return true;
      }
    case curv::Ref_Value::ty_record:
      {
        auto& record = (curv::Record&)ref;
        out << "{";
        bool first = true;
        for (auto i : record.fields_) {
            if (is_json_data(i.second)) {
                if (!first) out << ",";
                first = false;
                out << '"' << i.first << "\":";
                export_json_value(i.second, out);
            }
        }
        out << "}";
        return true;
      }
    default:
        return false;
    }
}
void export_json(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    curv::Output_File& ofile)
{
    for (auto& p : params.map) {
        params.unknown_parameter(p);
    }
    ofile.open();
    if (export_json_value(value, ofile.ostream()))
        ofile.ostream() << "\n";
    else {
        curv::At_Program cx(prog);
        throw curv::Exception(cx, "value can't be converted to JSON");
    }
}

const char export_png_help[] =
    "-O xsize=<image width in pixels>\n"
    "-O ysize=<image height in pixels>\n"
    "-O time=<animation frame timestamp, in seconds, default 0>\n"
    "-O aa=<supersampling factor for antialiasing; 1 means disabled>\n"
    "-O taa=<supersampling factor for temporal antialiasing; 1 means disabled>\n"
    "-O delay=<frame duration for animation (used with TAA)>\n";

void export_png(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    curv::Output_File& ofile)
{
    curv::geom::Image_Export ix;

    int xsize = 0;
    int ysize = 0;
    for (auto& p : params.map) {
        if (p.first == "xsize") {
            xsize = params.to_int(p, 1, INT_MAX);
        } else if (p.first == "ysize") {
            ysize = params.to_int(p, 1, INT_MAX);
        } else if (p.first == "time") {
            ix.time = params.to_double(p);
        } else if (p.first == "aa") {
            ix.aa_ = params.to_int(p, 1, INT_MAX);
        } else if (p.first == "taa") {
            ix.taa_ = params.to_int(p, 1, INT_MAX);
        } else if (p.first == "delay") {
            ix.delay_ = params.to_double(p);
        } else {
            params.unknown_parameter(p);
        }
    }

    curv::geom::Shape_Program shape(prog);
    curv::At_Program cx(prog);
    if (!shape.recognize(value))
        throw curv::Exception(cx, "not a shape");
    if (shape.is_2d_) {
        if (shape.bbox_.infinite2())
            throw curv::Exception(cx, "can't export an infinite 2D shape to PNG");
        if (shape.bbox_.empty2())
            throw curv::Exception(cx, "can't export an empty 2D shape to PNG");
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
    std::cerr << "Image export: "<<ix.size.x<<"×"<<ix.size.y<<" pixels."
        " Use 'curv --help -o png' for more options.\n";
    curv::geom::export_png(shape, ix, ofile);
}

std::map<std::string, Exporter> exporters = {
    {"curv", {export_curv, "Curv expression", ""}},
    {"stl", {export_stl, "STL mesh file (3D shape only)", mesh_export_help}},
    {"obj", {export_obj, "OBJ mesh file (3D shape only)", mesh_export_help}},
    {"x3d", {export_x3d, "X3D colour mesh file (3D shape only)",
             colour_mesh_export_help}},
    {"frag", {export_frag,
              "GLSL fragment shader (shape only, shadertoy.com compatible)",
              export_frag_help}},
    {"json", {export_json, "JSON expression", ""}},
    {"cpp", {export_cpp, "C++ source file (shape only)", ""}},
    {"png", {export_png, "PNG image file (shape only)", export_png_help}},
};
