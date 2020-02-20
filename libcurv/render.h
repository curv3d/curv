// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_RENDER_H
#define LIBCURV_RENDER_H

#include <libcurv/function.h>
#include <glm/vec3.hpp>
#include <vector>

namespace curv {

struct Record;
struct Context;
struct Function;

struct Render_Opts
{
    enum class Shader { standard, pew, sf1 };
    static const std::vector<const char*> shader_enum;

    // spatial anti-aliasing via supersampling. aa_==1 means it is turned off.
    int aa_ = 1;
    // temporal anti-aliasing.
    int taa_ = 1;
    // frame duration for animation, needed for TAA.
    double fdur_ = 0.04; // 25 FPS
    // background colour, defaults to white
    glm::dvec3 bg_ = glm::dvec3(1.0,1.0,1.0);
    // max # of iterations in the ray-marcher
    int ray_max_iter_ = 200;
    // max ray-marching distance
    double ray_max_depth_ = 400.0;
    // shader implementation
    Shader shader_ = Shader::standard;
    // sf1 shader function, configured as: shader={sf1:<function>}
    Shared<const Function> sf1_ = nullptr;

    void update_from_record(Record&, const Context&);
    static void describe_opts(std::ostream&, const char*);
    void set_shader(Value, const Context&);
    bool set_field(const std::string&, Value, const Context&);
};

} // namespace
#endif // header guard
