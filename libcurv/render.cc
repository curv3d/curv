// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/render.h>

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/function.h>
#include <libcurv/record.h>
#include <libcurv/shape.h>

#include <climits>

namespace curv {

const std::vector<const char*>
Render_Opts::shader_enum { "standard", "pew", "sf1" };

void
Render_Opts::set_shader(Value val, const Context& cx)
{
    auto v = value_to_variant(val, cx);
    if (v.first == "standard") {
        if (v.second.is_missing()) {
            shader_ = Shader::standard;
            return;
        }
        goto error;
    }
    if (v.first == "pew") {
        if (v.second.is_missing()) {
            shader_ = Shader::pew;
            return;
        }
        goto error;
    }
    if (v.first == "sf1") {
        shader_ = Shader::sf1;
        if (!v.second.is_missing()) {
            sf1_ = cast_to_function(v.second, cx);
            if (sf1_ == nullptr)
                goto error;
        }
        return;
    }
error:
    throw Exception(cx,
        stringify(val," is not #standard|#pew|{sf1:<function>}"));
}

void
Render_Opts::update_from_record(
    Record& r,
    const Context& cx)
{
    auto aa_val = r.find_field(make_symbol("aa"), cx);
    if (!aa_val.is_missing()) {
        aa_ = aa_val.to_int(1, INT_MAX, At_Field("aa", cx));
    }
    auto taa_val = r.find_field(make_symbol("taa"), cx);
    if (!taa_val.is_missing()) {
        taa_ = taa_val.to_int(1, INT_MAX, At_Field("taa", cx));
    }
    auto fdur_val = r.find_field(make_symbol("fdur"), cx);
    if (!fdur_val.is_missing()) {
        fdur_ = fdur_val.to_num(At_Field("fdur", cx));
    }
    auto bg_val = r.find_field(make_symbol("bg"), cx);
    if (!bg_val.is_missing()) {
        bg_ = value_to_vec3(bg_val, At_Field("bg", cx));
    }
    auto ray_max_iter_val = r.find_field(make_symbol("ray_max_iter"), cx);
    if (!ray_max_iter_val.is_missing()) {
        ray_max_iter_ = ray_max_iter_val.to_int(1, INT_MAX,
            At_Field("ray_max_iter", cx));
    }
    auto ray_max_depth_val = r.find_field(make_symbol("ray_max_depth"), cx);
    if (!ray_max_depth_val.is_missing()) {
        ray_max_depth_ = ray_max_depth_val.to_num(
            At_Field("ray_max_depth", cx));
    }
    auto shader_val = r.find_field(make_symbol("shader"), cx);
    if (!shader_val.is_missing()) {
        set_shader(shader_val, At_Field("shader", cx));
    }
}

void
Render_Opts::describe_opts(std::ostream& out, const char* prefix)
{
  Render_Opts opts;
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
  << prefix <<
  "-O shader=#standard|#pew|{sf1:<function>}\n"
  ;
}

bool
Render_Opts::set_field(const std::string& name, Value val, const Context& cx)
{
    if (name == "aa") {
        aa_ = val.to_int(1, INT_MAX, cx);
        return true;
    }
    if (name == "taa") {
        taa_ = val.to_int(1, INT_MAX, cx);
        return true;
    }
    if (name == "fdur") {
        fdur_ = val.to_num(cx);
        return true;
    }
    if (name == "bg") {
        bg_ = value_to_vec3(val, cx);
        return true;
    }
    if (name == "ray_max_iter") {
        ray_max_iter_ = val.to_int(1, INT_MAX, cx);
        return true;
    }
    if (name == "ray_max_depth") {
        ray_max_depth_ = val.to_num(cx);
        return true;
    }
    if (name == "shader") {
        set_shader(val, cx);
        return true;
    }
    return false;
}

} // namespace curv
