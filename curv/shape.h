// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#ifndef CURV_SHAPE_H
#define CURV_SHAPE_H

#include <curv/record.h>
#include <curv/gl_compiler.h>
#include <cmath>

namespace curv {

class Polyadic_Function;
class Context;

// axis aligned bounding box
struct BBox {
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

// TODO: a more compact/efficient representation for Shape?
// Maybe use the same internal representation as Record, with a different
// type tag?
// But, make_shape should also accept a module argument, and in that case,
// it should support customization (of the underlying module)?
// Actually I'm not sure if that will be supported.
// TODO: Shape should be abstract, user defined Shape should be a subclass.
// I presume that built-in shape classes don't need the record_ field.

struct Shape : public Ref_Value
{
    Shared<const Record> record_;

    Shape(Shared<const Record> record);

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const override;

    virtual Value getfield(Atom, const Context&) const override;
    virtual bool hasfield(Atom) const override;

    Polyadic_Function& dist(const Context&) const;
    BBox bbox(const Context&) const;

    /// Invoke the Geometry Compiler on the shape's `dist` function.
    GL_Value gl_dist(GL_Value, GL_Frame&) const;

    /// Invoke the Geometry Compiler on the shape's `colour` function.
    GL_Value gl_colour(GL_Value, GL_Frame&) const;

    static const char name[];
};

} // namespace curv
#endif // header guard
