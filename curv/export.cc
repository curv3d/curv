// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include "export.h"

#include <libcurv/geom/compiled_shape.h>
#include <libcurv/geom/export_frag.h>
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

void export_curv(curv::Value value,
    curv::Program&,
    const Export_Params&,
    curv::Output_File& ofile)
{
    ofile.open();
    ofile.ostream() << value << "\n";
}

void export_frag(curv::Value value,
    curv::Program& prog,
    const Export_Params&,
    curv::Output_File& ofile)
{
    curv::geom::Shape_Program shape(prog);
    if (!shape.recognize(value)) {
        curv::At_Program cx(prog);
        throw curv::Exception(cx, "not a shape");
    }
    ofile.open();
    curv::geom::export_frag(shape, ofile.ostream());
}

void export_cpp(curv::Value value,
    curv::Program& prog,
    const Export_Params&,
    curv::Output_File& ofile)
{
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
    const Export_Params&,
    curv::Output_File& ofile)
{
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
    "-O ysize=<image height in pixels>\n";

void export_png(curv::Value value,
    curv::Program& prog,
    const Export_Params& params,
    curv::Output_File& ofile)
{
    int xsize = 0;
    int ysize = 0;
    for (auto& p : params.map) {
        if (p.first == "xsize") {
            xsize = params.to_int(p, 1, INT_MAX);
        } else if (p.first == "ysize") {
            ysize = params.to_int(p, 1, INT_MAX);
        } else {
            params.unknown_parameter(p);
        }
    }

    curv::geom::Shape_Program shape(prog);
    curv::At_Program cx(prog);
    if (!shape.recognize(value) || !shape.is_2d_)
        throw curv::Exception(cx, "not a 2D shape");
    if (shape.bbox_.infinite2())
        throw curv::Exception(cx, "can't export an infinite shape to PNG");
    if (shape.bbox_.empty2())
        throw curv::Exception(cx, "can't export an empty shape to PNG");
    double dx = shape.bbox_.xmax - shape.bbox_.xmin;
    double dy = shape.bbox_.ymax - shape.bbox_.ymin;
    glm::ivec2 size;
    double pixsize;
    if (!xsize && !ysize) {
        if (dx > dy)
            xsize = 500;
        else
            ysize = 500;
    }
    if (xsize && !ysize) {
        size.x = xsize;
        pixsize = dx / double(xsize);
        size.y = (int) round(dy / dx * double(xsize));
        if (size.y == 0) ++size.y;
    } else if (!xsize && ysize) {
        size.y = ysize;
        pixsize = dy / double(ysize);
        size.x = (int) round(dx / dy * double(ysize));
        if (size.x == 0) ++size.x;
    } else {
        size.x = xsize;
        size.y = ysize;
        pixsize = std::min(dx / double(xsize), dy / double(ysize));
    }
    std::cerr << "Image export: "<<size.x<<"×"<<size.y<<" pixels."
        " Use 'curv --help -o png' for help.\n";
    curv::geom::export_png(shape, size, pixsize, ofile);
}

std::map<std::string, Exporter> exporters = {
    {"curv", {export_curv, "Curv expression", ""}},
    {"stl", {export_stl, "STL mesh file (3D shape only)", mesh_export_help}},
    {"obj", {export_obj, "OBJ mesh file (3D shape only)", mesh_export_help}},
    {"x3d", {export_x3d, "X3D colour mesh file (3D shape only)",
             colour_mesh_export_help}},
    {"frag", {export_frag,
              "GLSL fragment shader (shape only, shadertoy.com compatible)",
              ""}},
    {"json", {export_json, "JSON expression", ""}},
    {"cpp", {export_cpp, "C++ source file (shape only)", ""}},
    {"png", {export_png, "PNG image file (2D shape only)", export_png_help}},
};
