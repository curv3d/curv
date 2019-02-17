// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_COMPILED_SHAPE_H
#define LIBCURV_GEOM_COMPILED_SHAPE_H

#include <libcurv/shape.h>
#include <ostream>
#include <glm/vec3.hpp>

namespace curv { namespace geom {

struct Compiled_Shape final : public Shape
{
    double (*dist_)(double,double,double,double);
    void (*colour_)(double,double,double,double,glm::vec3*);

    Compiled_Shape(Shape_Program&);

    virtual double dist(double x, double y, double z, double t) override
    {
        return dist_(x,y,z,t);
    }
    virtual Vec3 colour(double x, double y, double z, double t) override
    {
        glm::vec3 c;
        colour_(x,y,z,t, &c);
        return Vec3{c.x,c.y,c.z};
    }
};

void export_cpp(Shape_Program& shape, std::ostream& out);

}} // namespace
#endif // include guard
