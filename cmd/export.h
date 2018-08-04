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

struct Export_Params
{
    using Map = std::map<std::string, std::string>;
    std::string format;
    Map map;
};

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

#endif // include guard
