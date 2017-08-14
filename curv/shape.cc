// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENCE.md or https://opensource.org/licenses/MIT

#include <cmath>

#include <curv/shape.h>
#include <curv/exception.h>
#include <curv/context.h>
#include <curv/gl_context.h>
#include <curv/function.h>

namespace curv {

const char Shape::name[] = "shape";

struct Blackfield_Function : public Polyadic_Function
{
    Blackfield_Function() : Polyadic_Function(1) {}
    Value call(Frame& args) override
    {
        Shared<List> v = List::make({Value{0.8}, Value{0.8}, Value{0.5}});
        return {v};
    }
    GL_Value gl_call(GL_Frame& f) const override
    {
        auto result = f.gl.newvalue(GL_Type::Vec3);
        f.gl.out << "  vec3 "<<result<<" = vec3(0.8,0.8,0.5);\n";
        return result;
    }
};

Shape::Shape(Shared<const Structure> s, const Context& cx)
:
    Structure(ty_shape), fields_()
{
    // TODO: future optimization.
    // If `s` is a unique Record ref, move its contents into the Shape.
    // Ditto if arg is a unique Shape ref. Otherwise, copy the fields.
    s->putfields(fields_);

    static Atom colour = "colour";
    static Value black = {make<Blackfield_Function>()};
    if (fields_.find(colour) == fields_.end()) {
        fields_[colour] = black;
    }

    static Atom bbox_key = "bbox";
    static Value infbbox = {List::make({
        {List::make({ Value(-INFINITY), Value(-INFINITY), Value(-INFINITY) })},
        {List::make({ Value(+INFINITY), Value(+INFINITY), Value(+INFINITY) })},
    })};
    auto bbox_p = fields_.find(bbox_key);
    if (bbox_p == fields_.end()) {
        fields_[bbox_key] = infbbox;
    }

    static Atom is_2d_key = "is_2d";
    bool is_2d;
    auto is_2d_p = fields_.find(is_2d_key);
    if (is_2d_p == fields_.end()) {
        fields_[is_2d_key] = {false};
        is_2d = false;
    } else {
        is_2d = is_2d_p->second.to_bool(At_Field("is_2d", cx));
    }

    static Atom is_3d_key = "is_3d";
    bool is_3d;
    auto is_3d_p = fields_.find(is_3d_key);
    if (is_3d_p == fields_.end()) {
        fields_[is_3d_key] = {false};
        is_3d = false;
    } else {
        is_3d = is_3d_p->second.to_bool(At_Field("is_3d", cx));
    }

    if (!is_2d && !is_3d)
        throw Exception(cx,
            "make_shape: at least one of is_2d and is_3d must be true");
}

BBox
BBox::from_value(Value val, const Context& cx)
{
    auto list = val.to<List>(cx);
    list->assert_size(2, cx);

    At_Index mincx(0, cx);
    auto mins = list->at(0).to<List>(mincx);
    mins->assert_size(3, mincx);

    At_Index maxcx(1, cx);
    auto maxs = list->at(1).to<List>(maxcx);
    maxs->assert_size(3, maxcx);

    BBox b;
    b.xmin = mins->at(0).to_num(cx);
    b.ymin = mins->at(1).to_num(cx);
    b.zmin = mins->at(2).to_num(cx);
    b.xmax = maxs->at(0).to_num(cx);
    b.ymax = maxs->at(1).to_num(cx);
    b.zmax = maxs->at(2).to_num(cx);
    return b;
}

void
Shape::print(std::ostream& out) const
{
    out << "make_shape{";
    bool first = true;
    for (auto i : fields_) {
        if (!first) out << ",";
        first = false;
        out << i.first << ":";
        i.second.print(out);
    }
    out << "}";
}

Value
Shape::getfield(Atom name, const Context& cx) const
{
    auto fp = fields_.find(name);
    if (fp != fields_.end())
        return fp->second;
    return Structure::getfield(name, cx);
}

bool
Shape::hasfield(Atom name) const
{
    auto fp = fields_.find(name);
    return (fp != fields_.end());
}

void
Shape::putfields(Atom_Map<Value>& out) const
{
    for (auto i : fields_)
        out[i.first] = i.second;
}

size_t
Shape::size() const
{
    return fields_.size();
}

Shared<List>
Shape::dom() const
{
    auto list = List::make(fields_.size());
    int i = 0;
    for (auto f : fields_) {
        list->at(i) = f.first.to_value();
        ++i;
    }
    return list;
}

Polyadic_Function&
Shape::dist(const Context& cx) const
{
    auto val = getfield("dist", cx);
    auto fun = val.dycast<Polyadic_Function>();
    if (fun == nullptr)
        throw Exception(cx, "Shape: dist is not a function");
    if (fun->nargs_ != 1 && fun->nargs_ != 4)
        throw Exception(cx, "Shape: dist function must have 1 or 4 parameters");
    return *fun;
}

BBox
Shape::bbox(const Context& cx) const
{
    return BBox::from_value(getfield("bbox",cx), At_Field("bbox", cx));
}

GL_Value
Shape::gl_dist(GL_Value arg, GL_Frame& f) const
{
    if (arg.type != GL_Type::Vec4)
        throw Exception(At_GL_Frame(&f), stringify(
            "dist function argument must be vec4, is ", arg.type));
    Polyadic_Function& fun = dist(At_GL_Frame(&f));
    auto f2 = GL_Frame::make(fun.nslots_, f.gl, nullptr, &f, nullptr);
    if (fun.nargs_ == 1) {
        (*f2)[0] = arg;
    } else if (fun.nargs_ == 4) {
        (*f2)[0] = gl_vec_element(f, arg, 0);
        (*f2)[1] = gl_vec_element(f, arg, 1);
        (*f2)[2] = gl_vec_element(f, arg, 2);
        (*f2)[3] = gl_vec_element(f, arg, 3);
    } else assert(0);
    auto result = fun.gl_call(*f2);
    if (result.type != GL_Type::Num) {
        throw Exception(At_GL_Frame(&f),
            stringify("dist function returns ",result.type));
    }
    return result;
}

GL_Value
Shape::gl_colour(GL_Value arg, GL_Frame& f) const
{
    if (arg.type != GL_Type::Vec4)
        throw Exception(At_GL_Frame(&f), stringify(
            "colour function argument must be vec4, is ", arg.type));
    At_GL_Frame cx(&f);
    auto fun = getfield("colour", cx).to<Polyadic_Function>(cx);
    auto f2 = GL_Frame::make(fun->nslots_, f.gl, nullptr, &f, nullptr);
    if (fun->nargs_ == 1) {
        (*f2)[0] = arg;
    } else if (fun->nargs_ == 4) {
        (*f2)[0] = gl_vec_element(f, arg, 0);
        (*f2)[1] = gl_vec_element(f, arg, 1);
        (*f2)[2] = gl_vec_element(f, arg, 2);
        (*f2)[3] = gl_vec_element(f, arg, 3);
    } else assert(0);
    auto result = fun->gl_call(*f2);
    if (result.type != GL_Type::Vec3) {
        throw Exception(cx, stringify("colour function returns ",result.type));
    }
    return result;
}

} // namespace curv
