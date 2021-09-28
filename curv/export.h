// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef EXPORT_H
#define EXPORT_H

#include "config.h"
#include <libcurv/context.h>
#include <libcurv/output_file.h>
#include <libcurv/program.h>
#include <libcurv/symbol.h>
#include <libcurv/value.h>
#include <glm/vec3.hpp>
#include <iostream>
#include <map>
#include <string>

namespace curv { namespace viewer {
struct Viewer_Config;
}}

struct Export_Params
{
    using Options = std::map<std::string, curv::Shared<const curv::String>>;
    struct PValue {
        curv::Shared<const curv::String> opt;
        curv::Value config;
    };
    using Map = std::map<std::string, PValue>;

    Export_Params(
        Options options,
        const Config& config,
        curv::System& sys);

    const Config& config_;
    curv::System& system_;
    std::string format_;
    Map map_;
    bool verbose_ = false;
};

struct Param : public curv::Context
{
    Param(
        const Export_Params& params,
        const Export_Params::Map::value_type& p)
    :
        params_(params),
        name_(p.first),
        value_(p.second),
        loc_()
    {}

    const Export_Params& params_;
    const std::string& name_;
    const Export_Params::PValue& value_;
    curv::Src_Loc loc_;

    // First, ye must evaluate the parameter.
    // If the parameter was specified as a command line option `-Ofoo`
    // (without `=value` at the end) and `defl` is not missing,
    // then `defl` is returned.
    curv::Value eval(curv::Value defl = curv::missing);

    // These are high level wrappers that call eval() and then validate
    // the parameter value.
    int to_int(int, int);
    double to_double();
    double to_double(double);
    glm::dvec3 to_vec3();
    bool to_bool();
    curv::Symbol_Ref to_symbol();
    int to_enum(const std::vector<const char*>& e);

    void unknown_parameter() const;

    // After eval() is called, ye may report a bad value by throwing
    // an Exception, using the Param as a context. But throw not
    // before calling eval(), because the Param is not yet initialized
    // as a Context.
    virtual void get_locations(std::list<curv::Func_Loc>&) const override;
    virtual curv::Shared<const curv::String> rewrite_message(
        curv::Shared<const curv::String>) const override;
    virtual curv::System& system() const override;
    virtual curv::Frame* frame() const override;
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

extern void export_gltf(curv::Value,
    curv::Program&,
    const Export_Params& params,
    curv::Output_File&);

extern void export_json(curv::Value value,
    curv::Program&,
    const Export_Params& params,
    curv::Output_File&);

extern void export_gpu(curv::Value,
    curv::Program&, const Export_Params&, curv::Output_File&);

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
    curv::viewer::Viewer_Config& opts);
void describe_viewer_options(std::ostream&, const char* prefix="");

#endif // include guard
