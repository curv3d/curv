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

bool
Shape_Recognizer::recognize(Value val)
{
    static Atom is_2d_key = "is_2d";
    static Atom is_3d_key = "is_3d";
    static Atom bbox_key = "bbox";
    static Atom dist_key = "dist";
    static Atom colour_key = "colour";

    Value is_2d_val;
    Value is_3d_val;
    Value bbox_val;
    Value dist_val;
    Value colour_val;

    auto s = val.dycast<Structure>();
    if (s == nullptr)
        return false;
    if (s->hasfield(is_2d_key))
        is_2d_val = s->getfield(is_2d_key, context_);
    else
        return false;
    if (s->hasfield(is_3d_key))
        is_3d_val = s->getfield(is_3d_key, context_);
    else
        return false;
    if (s->hasfield(bbox_key))
        bbox_val = s->getfield(bbox_key, context_);
    else
        return false;
    if (s->hasfield(dist_key))
        dist_val = s->getfield(dist_key, context_);
    else
        return false;
    if (s->hasfield(colour_key))
        colour_val = s->getfield(colour_key, context_);
    else
        return false;

    is_2d_ = is_2d_val.to_bool(At_Field("is_2d", context_));
    is_3d_ = is_3d_val.to_bool(At_Field("is_3d", context_));
    if (!is_2d_ && !is_3d_)
        throw Exception(context_,
            "at least one of is_2d and is_3d must be true");
    bbox_ = BBox::from_value(bbox_val, At_Field("bbox", context_));
    dist_ = dist_val.dycast<Function>();
    if (dist_ == nullptr)
        throw Exception(At_Field("dist", context_), "dist is not a function");
    colour_ = colour_val.dycast<Function>();
    if (colour_ == nullptr)
        throw Exception(At_Field("colour", context_),
            "colour is not a function");

    return true;
}

struct GL_Data_Ref : public Operation
{
    GL_Value val_;
    GL_Data_Ref(Shared<const Phrase> src, GL_Value v)
    : Operation(std::move(src)), val_(v)
    {}
    GL_Value gl_eval(GL_Frame&) const override { return val_; }
};

GL_Value
Shape_Recognizer::gl_dist(GL_Value arg, GL_Compiler& gl) const
{
    assert(arg.type == GL_Type::Vec4);
    const At_Field cx("dist", context_);
    auto f = GL_Frame::make(0, gl, &cx, nullptr, nullptr);
    auto aref = make<GL_Data_Ref>(nullptr, arg);
    auto result = dist_->gl_call_expr(*aref, nullptr, *f);
    if (result.type != GL_Type::Num)
        throw Exception(cx, stringify("dist function returns ",result.type));
    return result;
}

GL_Value
Shape_Recognizer::gl_colour(GL_Value arg, GL_Compiler& gl) const
{
    assert(arg.type == GL_Type::Vec4);
    const At_Field cx("colour", context_);
    auto f = GL_Frame::make(0, gl, &cx, nullptr, nullptr);
    auto aref = make<GL_Data_Ref>(nullptr, arg);
    auto result = colour_->gl_call_expr(*aref, nullptr, *f);
    if (result.type != GL_Type::Vec3)
        throw Exception(cx, stringify("colour function returns ",result.type));
    return result;
}

} // namespace curv
