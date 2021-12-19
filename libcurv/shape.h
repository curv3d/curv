// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SHAPE_H
#define LIBCURV_SHAPE_H

#include <libcurv/frame.h>
#include <libcurv/function.h>
#include <libcurv/location.h>
#include <libcurv/vec.h>
#include <cmath>

namespace curv {

struct Context;
struct System;
struct Program;
struct Phrase;
struct Render_Opts;

struct Viewed_Shape;

// axis aligned bounding box
struct BBox
{
    glm::dvec3 min, max;
    BBox() {}
    BBox(glm::dvec3 bmin, glm::dvec3 bmax) : min(bmin), max(bmax) {}
    bool empty2() const {
        return (min.x >= max.x || min.y >= max.y);
    }
    bool empty3() const {
        return (min.x >= max.x || min.y >= max.y || min.z >= max.z);
    }
    bool infinite2() const {
        return (min.x == -INFINITY || min.y == -INFINITY ||
                max.x == +INFINITY || max.y == +INFINITY);
    }
    bool infinite3() const {
        return min.x == -INFINITY || min.y == -INFINITY || min.z == -INFINITY
            || max.x == +INFINITY || max.y == +INFINITY || max.z == +INFINITY;
    }
    glm::dvec2 size2() const {
        return glm::dvec2(max.x - min.x, max.y - min.y);
    }
    glm::dvec3 size3() const {
        return max - min;
    }
    static BBox from_value(Value, const Context&);
};

struct Shape
{
    bool is_2d_;
    bool is_3d_;
    BBox bbox_;
    virtual double dist(double x, double y, double z, double t) const = 0;
    virtual Vec3 colour(double x, double y, double z, double t) const = 0;
};

struct Shape_Program final : public Shape
{
    // is_shape is initially false, becomes true after recognize() succeeds.
    bool is_shape() const { return is_2d_ || is_3d_; }

    // implement PROGRAM api for use with At_Program
    Source_State& sstate_;
    Shared<const Phrase> nub_; // source code of shape expression
    System& system() const { return sstate_.system_; }
    const Phrase& syntax() const { return *nub_; }

    // shape fields, filled in by recognize()
    Shared<Record> record_;
    Shared<const Function> dist_fun_;
    Shared<const Function> colour_fun_;
    std::unique_ptr<Frame> dist_frame_;
    std::unique_ptr<Frame> colour_frame_;

    Viewed_Shape* viewed_shape_ = nullptr;

    Shape_Program(Program&);

    Shape_Program(Source_State& sstate, Shared<const Phrase> nub)
    : sstate_(sstate), nub_(std::move(nub))
    {}

    // If the value is a shape, fill in the shape fields and return true.
    // The Render_Opts argument is modified using data from shape.render,
    // if the recognized shape has a render field.
    // Used with the first constructor.
    bool recognize(Value, Render_Opts*);

    // This is called from the Viewed_Shape constructor, after a
    // parametric shape has been recognized. We construct a Shape_Program
    // that describes a parametric shape.
    Shape_Program(const Shape_Program&, Shared<Record>, Viewed_Shape*);

    // Invoke the shape's `dist` function.
    double dist(double x, double y, double z, double t) const;

    // Invoke the shape's `colour` function.
    Vec3 colour(double x, double y, double z, double t) const;
};

} // namespace
#endif // header guard
