// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#ifndef CURV_SHAPE_H
#define CURV_SHAPE_H

#include <curv/record.h>
#include <curv/gl_compiler.h>
#include <cmath>

namespace curv {

class Function;
class Context;

// axis aligned bounding box
struct BBox {
    double xmin, ymin;
    double xmax, ymax;
    bool empty() {
        return (xmin >= xmax || ymin >= ymax);
    }
    bool infinite() {
        return (xmin == -INFINITY || ymin == -INFINITY ||
                xmax == +INFINITY || ymax == +INFINITY);
    }
    static BBox from_value(Value, const Context&);
};

// TODO: a more compact/efficient representation for Shape2D?
// Maybe use the same internal representation as Record, with a different
// type tag?
// But, shape2d should also accept a module argument, and in that case,
// it should support customization (of the underlying module)?
// Actually I'm not sure if that will be supported.
// TODO: Shape2D should be abstract, user defined Shape2D should be a subclass.
// I presume that built-in shape classes don't need the record_ field.

struct Shape2D : public Ref_Value
{
    Shared<const Record> record_;

    Shape2D(Shared<const Record> record);

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const override;

    virtual Value getfield(Atom) const override;

    Function& dist(const Context&) const;
    BBox bbox(const Context&) const;

    /// Invoke the Geometry Compiler on the shape's `dist` function.
    GL_Value gl_dist(GL_Value, GL_Frame&) const;

    /// Invoke the Geometry Compiler on the shape's `colour` function.
    GL_Value gl_colour(GL_Value, GL_Frame&) const;
};

} // namespace curv
#endif // header guard
