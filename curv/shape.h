// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef CURV_SHAPE_H
#define CURV_SHAPE_H

#include <curv/record.h>
#include <curv/gl_compiler.h>
#include <cmath>

namespace curv {

struct Function;
struct Context;

// axis aligned bounding box
struct BBox
{
    double xmin, ymin, zmin;
    double xmax, ymax, zmax;
    bool empty() {
        return (xmin >= xmax || ymin >= ymax);
    }
    bool infinite() {
        return (xmin == -INFINITY || ymin == -INFINITY || zmin == -INFINITY ||
                xmax == +INFINITY || ymax == +INFINITY || zmax == +INFINITY);
    }
    static BBox from_value(Value, const Context&);
};

struct Shape_Recognizer
{
    // describes the source code for the shape expression
    const Context& context_;

    // shape fields, filled in by recognize()
    bool is_2d_;
    bool is_3d_;
    BBox bbox_;
    Shared<Function> dist_;
    Shared<Function> colour_;

    Shape_Recognizer(const Context& cx) : context_(cx) {}

    // If the value is a shape, fill in the shape fields and return true.
    bool recognize(Value);

    /// Invoke the Geometry Compiler on the shape's `dist` function.
    GL_Value gl_dist(GL_Value, GL_Compiler&) const;

    /// Invoke the Geometry Compiler on the shape's `colour` function.
    GL_Value gl_colour(GL_Value, GL_Compiler&) const;
};

} // namespace curv
#endif // header guard
