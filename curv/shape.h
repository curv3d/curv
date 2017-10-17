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

struct Shape : public Structure
{
    Atom_Map<Value> fields_;

    Shape(Shared<const Structure>, const Context&);

    /// Print a value like a Curv expression.
    virtual void print(std::ostream&) const override;

    virtual Value getfield(Atom, const Context&) const override;
    virtual bool hasfield(Atom) const override;
    virtual void putfields(Atom_Map<Value>&) const override;
    virtual size_t size() const override;
    virtual Shared<List> fields() const override;

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
