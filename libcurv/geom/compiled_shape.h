// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_COMPILED_SHAPE_H
#define LIBCURV_GEOM_COMPILED_SHAPE_H

#include <libcurv/shape.h>
#include <ostream>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace curv { namespace geom {

struct Compiled_Shape final : public Shape
{
    void (*dist_)(const glm::vec4* in, float* out);
    void (*colour_)(const glm::vec4* in, glm::vec3* out);

    Compiled_Shape(Shape_Program&);

    virtual double dist(double x, double y, double z, double t) override
    {
        glm::vec4 in{x,y,z,t};
        float out;
        dist_(&in, &out);
        return out;
    }
    virtual Vec3 colour(double x, double y, double z, double t) override
    {
        glm::vec4 in{x,y,z,t};
        glm::vec3 out;
        colour_(&in, &out);
        return Vec3{out.x,out.y,out.z};
    }
};

void export_cpp(Shape_Program& shape, std::ostream& out);

}} // namespace
#endif // include guard
