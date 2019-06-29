// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/render.h>

#include <libcurv/context.h>
#include <libcurv/function.h>
#include <libcurv/record.h>
#include <libcurv/shape.h>

#include <climits>

namespace curv {

const std::vector<const char*>
Render_Opts::shader_enum { "standard", "pew" };

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
        shader_ = (Render_Opts::Shader) value_to_enum(shader_val,
            shader_enum, At_Field("shader", cx));
    }
}

} // namespace curv
