// Copyright Doug Moen 2016-2017.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

#include <curv/shape.h>
#include <curv/exception.h>
#include <curv/context.h>
#include <curv/gl_context.h>
#include <curv/function.h>

namespace curv {

const char Shape2D::name[] = "shape";

struct Blackfield_Function : public Polyadic_Function
{
    Blackfield_Function() : Polyadic_Function(1) {}
    Value call(Frame& args) override
    {
        Shared<List> v = List::make({Value{0.0}, Value{0.0}, Value{0.0}});
        return {v};
    }
    GL_Value gl_call(GL_Frame& f) const override
    {
        auto result = f.gl.newvalue(GL_Type::Vec3);
        f.gl.out << "  vec3 "<<result<<" = vec3(0.8,0.8,0.5);\n";
        return result;
    }
};

Shape2D::Shape2D(Shared<const Record> record)
:
    Ref_Value(ty_shape2d), record_(std::move(record))
{
    static Atom colour = "colour";
    static Value black = {make<Blackfield_Function>()};
    auto& fields = record_->fields_;
    if (fields.find(colour) == fields.end()) {
        auto& u = update_shared(record_);
        u.fields_[colour] = black;
    }
}

BBox
BBox::from_value(Value val, const Context& cx)
{
    auto list = val.to<List>(cx);
    list->assert_size(2, cx);
    auto mins = list->at(0).to<List>(cx);
    mins->assert_size(2, cx);
    auto maxs = list->at(1).to<List>(cx);
    maxs->assert_size(2, cx);
    BBox b;
    b.xmin = mins->at(0).to_num(cx);
    b.ymin = mins->at(1).to_num(cx);
    b.xmax = maxs->at(0).to_num(cx);
    b.ymax = maxs->at(1).to_num(cx);
    return b;
}

void
Shape2D::print(std::ostream& out) const
{
    out << "shape2d";
    record_->print(out);
}

Value
Shape2D::getfield(Atom name, const Context& cx) const
{
    return record_->getfield(name, cx);
}

bool
Shape2D::hasfield(Atom name) const
{
    return record_->hasfield(name);
}

Polyadic_Function&
Shape2D::dist(const Context& cx) const
{
    auto val = getfield("dist", cx);
    auto fun = val.dycast<Polyadic_Function>();
    if (fun == nullptr)
        throw Exception(cx, "Shape2D: dist is not a function");
    if (fun->nargs_ != 1)
        throw Exception(cx, "Shape2D: dist function does not have 1 parameter");
    return *fun;
}

BBox
Shape2D::bbox(const Context& cx) const
{
    return BBox::from_value(getfield("bbox",cx), cx);
}

GL_Value
Shape2D::gl_dist(GL_Value arg, GL_Frame& f) const
{
    Polyadic_Function& fun = dist(At_GL_Frame(&f));
    auto f2 = GL_Frame::make(fun.nslots_, f.gl, &f, nullptr);
    (*f2)[0] = arg;
    auto result = fun.gl_call(*f2);
    if (result.type != GL_Type::Num) {
        throw Exception(At_GL_Frame(&f),
            stringify("dist function returns ",result.type));
    }
    return result;
}

GL_Value
Shape2D::gl_colour(GL_Value arg, GL_Frame& f) const
{
    At_GL_Frame cx(&f);
    auto fun = getfield("colour", cx).to<Polyadic_Function>(cx);
    auto f2 = GL_Frame::make(fun->nslots_, f.gl, &f, nullptr);
    (*f2)[0] = arg;
    auto result = fun->gl_call(*f2);
    if (result.type != GL_Type::Vec3) {
        throw Exception(cx, stringify("colour function returns ",result.type));
    }
    return result;
}

} // namespace curv
