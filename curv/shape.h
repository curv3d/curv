// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_SHAPE_H
#define CURV_SHAPE_H

#include <curv/record.h>
#include <curv/gl_compiler.h>
#include <cmath>

namespace curv {

struct Polyadic_Function;
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
        return (xmin == -INFINITY || ymin == -INFINITY ||
                xmax == +INFINITY || ymax == +INFINITY);
    }
    static BBox from_value(Value, const Context&);
};

struct Shape_Recognizer
{
    // shape fields, filled in by recognize()
    bool is_2d_;
    bool is_3d_;
    BBox bbox_;
    Shared<Polyadic_Function> dist_;
    Shared<Polyadic_Function> colour_;

    // If the value is a shape, fill in the shape fields and return true.
    bool recognize(Value, const Context&);

    /// Invoke the Geometry Compiler on the shape's `dist` function.
    GL_Value gl_dist(GL_Value, GL_Frame&) const;

    /// Invoke the Geometry Compiler on the shape's `colour` function.
    GL_Value gl_colour(GL_Value, GL_Frame&) const;
};

} // namespace curv
#endif // header guard
