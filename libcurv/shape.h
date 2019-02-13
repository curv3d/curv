// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SHAPE_H
#define LIBCURV_SHAPE_H

#include <libcurv/frame.h>
#include <libcurv/gl_compiler.h>
#include <libcurv/location.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <cmath>

namespace curv {

struct Function;
struct Context;
struct System;
struct Program;
struct Phrase;

struct Viewed_Shape;
using Vec3 = glm::dvec3;

// axis aligned bounding box
struct BBox
{
    double xmin, ymin, zmin;
    double xmax, ymax, zmax;
    bool empty2() const {
        return (xmin >= xmax || ymin >= ymax);
    }
    bool empty3() const {
        return (xmin >= xmax || ymin >= ymax || zmin >= zmax);
    }
    bool infinite2() const {
        return (xmin == -INFINITY || ymin == -INFINITY ||
                xmax == +INFINITY || ymax == +INFINITY);
    }
    bool infinite3() const {
        return (xmin == -INFINITY || ymin == -INFINITY || zmin == -INFINITY ||
                xmax == +INFINITY || ymax == +INFINITY || zmax == +INFINITY);
    }
    glm::dvec2 size2() const {
        return glm::dvec2(xmax - xmin, ymax - ymin);
    }
    glm::dvec3 size3() const {
        return glm::dvec3(xmax - xmin, ymax - ymin, zmax - zmin);
    }
    static BBox from_value(Value, const Context&);
};

struct Shape
{
    bool is_2d_;
    bool is_3d_;
    BBox bbox_;
    virtual double dist(double x, double y, double z, double t) = 0;
    virtual Vec3 colour(double x, double y, double z, double t) = 0;
};

struct Shape_Program final : public Shape
{
    // is_shape is initially false, becomes true after recognize() succeeds.
    bool is_shape() const { return is_2d_ || is_3d_; }

    System& system_;

    // describes the source code for the shape expression
    Shared<const Phrase> nub_;

    Location location() const;
    System& system() const { return system_; }
    Frame* file_frame() const { return nullptr; }

    // shape fields, filled in by recognize()
    Shared<Record> record_;
    Shared<Function> dist_fun_;
    Shared<Function> colour_fun_;
    std::unique_ptr<Frame> dist_frame_;
    std::unique_ptr<Frame> colour_frame_;

    Viewed_Shape* viewed_shape_ = nullptr;

    Shape_Program(Program&);

    Shape_Program(System& sys, Shared<const Phrase> nub)
    : system_(sys), nub_(std::move(nub))
    {}

    // If the value is a shape, fill in the shape fields and return true.
    // Used with the first constructor.
    bool recognize(Value);

    // This is called from the Viewed_Shape constructor, after a
    // parametric shape has been recognized. We construct a Shape_Program
    // that describes a parametric shape.
    Shape_Program(const Shape_Program&, Shared<Record>, Viewed_Shape*);

    /// Invoke the Geometry Compiler on the shape's `dist` function.
    GL_Value gl_dist(GL_Value, GL_Compiler&) const;

    /// Invoke the Geometry Compiler on the shape's `colour` function.
    GL_Value gl_colour(GL_Value, GL_Compiler&) const;

    // Invoke the shape's `dist` function.
    double dist(double x, double y, double z, double t);

    // Invoke the shape's `colour` function.
    Vec3 colour(double x, double y, double z, double t);
};

} // namespace
#endif // header guard
