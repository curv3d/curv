// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef EXPORT_H
#define EXPORT_H

#include <iostream>
#include <map>
#include <string>
#include <libcurv/output_file.h>
#include <libcurv/program.h>
#include <libcurv/value.h>

namespace curv { namespace geom { namespace viewer {
struct Viewer_Config;
}}}

struct Export_Params
{
    Export_Params(curv::System& sys) : system_(sys) {}
    using Map = std::map<std::string, std::string>;
    curv::System& system_;
    std::string format_;
    Map map_;
    bool verbose_ = false;
    [[noreturn]] void unknown_parameter(const Map::value_type&) const;
    int to_int(const Map::value_type&, int, int) const;
    double to_double(const Map::value_type&) const;
};

struct Exporter
{
    void (*call)(curv::Value, curv::Program&, const Export_Params&, curv::Output_File&);
    const char* synopsis;
    void (*describe_options)(std::ostream&);
};
extern std::map<std::string, Exporter> exporters;

extern void export_curv(curv::Value value,
    curv::Program&,
    const Export_Params& params,
    curv::Output_File&);

extern void export_stl(curv::Value,
    curv::Program&,
    const Export_Params& params,
    curv::Output_File&);

extern void export_obj(curv::Value,
    curv::Program&,
    const Export_Params& params,
    curv::Output_File&);

extern void export_x3d(curv::Value,
    curv::Program&,
    const Export_Params& params,
    curv::Output_File&);

extern void export_frag(curv::Value value,
    curv::Program&,
    const Export_Params& params,
    curv::Output_File&);

extern void export_json(curv::Value value,
    curv::Program&,
    const Export_Params& params,
    curv::Output_File&);

extern void export_cpp(curv::Value value,
    curv::Program&,
    const Export_Params& params,
    curv::Output_File&);

extern void export_png(curv::Value value,
    curv::Program&,
    const Export_Params& params,
    curv::Output_File&);

void describe_mesh_opts(std::ostream&);
void describe_colour_mesh_opts(std::ostream&);

void parse_viewer_config(
    const Export_Params& params,
    curv::geom::viewer::Viewer_Config& opts);
void describe_viewer_options(std::ostream&, const char* prefix="");

#endif // include guard
