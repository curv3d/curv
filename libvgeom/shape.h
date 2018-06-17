// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBVGEOM_SHAPE_H
#define LIBVGEOM_SHAPE_H

#include <libcurv/record.h>
#include <libcurv/gl_compiler.h>
#include <libcurv/frame.h>
#include <cmath>

namespace curv {
struct Function;
struct Context;
struct System;
}

namespace vgeom {

struct Vec3
{
    double x, y, z;
};

// axis aligned bounding box
struct BBox
{
    double xmin, ymin, zmin;
    double xmax, ymax, zmax;
    bool empty2() {
        return (xmin >= xmax || ymin >= ymax);
    }
    bool empty3() {
        return (xmin >= xmax || ymin >= ymax || zmin >= zmax);
    }
    bool infinite2() {
        return (xmin == -INFINITY || ymin == -INFINITY ||
                xmax == +INFINITY || ymax == +INFINITY);
    }
    bool infinite3() {
        return (xmin == -INFINITY || ymin == -INFINITY || zmin == -INFINITY ||
                xmax == +INFINITY || ymax == +INFINITY || zmax == +INFINITY);
    }
    static BBox from_value(curv::Value, const curv::Context&);
};

struct Shape
{
    bool is_2d_;
    bool is_3d_;
    BBox bbox_;
    virtual double dist(double x, double y, double z, double t) = 0;
    virtual Vec3 colour(double x, double y, double z, double t) = 0;
};

struct Shape_Recognizer final : public Shape
{
    // describes the source code for the shape expression
    const curv::Context& context_;

    curv::System& system_;

    // shape fields, filled in by recognize()
    curv::Shared<curv::Function> dist_fun_;
    curv::Shared<curv::Function> colour_fun_;
    std::unique_ptr<curv::Frame> dist_frame_;
    std::unique_ptr<curv::Frame> colour_frame_;

    Shape_Recognizer(const curv::Context& cx, curv::System& sys)
    :
        context_(cx),
        system_(sys)
    {}

    // If the value is a shape, fill in the shape fields and return true.
    bool recognize(curv::Value);

    /// Invoke the Geometry Compiler on the shape's `dist` function.
    curv::GL_Value gl_dist(curv::GL_Value, curv::GL_Compiler&) const;

    /// Invoke the Geometry Compiler on the shape's `colour` function.
    curv::GL_Value gl_colour(curv::GL_Value, curv::GL_Compiler&) const;

    // Invoke the shape's `dist` function.
    double dist(double x, double y, double z, double t);

    // Invoke the shape's `colour` function.
    Vec3 colour(double x, double y, double z, double t);
};

} // namespace vgeom
#endif // header guard
