// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/shape.h>

#include <libcurv/exception.h>
#include <libcurv/context.h>
#include <libcurv/gl_context.h>
#include <libcurv/function.h>
#include <libcurv/frame.h>
#include <libcurv/program.h>
#include <cmath>

namespace curv { namespace geom {

Shape_Program::Shape_Program(
    Program& prog)
:
    nub_(nub_phrase(prog.phrase_)),
    system_(prog.system())
{
    // mark initial state (no shape has been recognized yet)
    is_2d_ = false;
    is_3d_ = false;
}

Location Shape_Program::location() const
{
    return nub_->location();
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

bool
Shape_Program::recognize(Value val)
{
    static Symbol is_2d_key = "is_2d";
    static Symbol is_3d_key = "is_3d";
    static Symbol bbox_key = "bbox";
    static Symbol dist_key = "dist";
    static Symbol colour_key = "colour";

    Value is_2d_val;
    Value is_3d_val;
    Value bbox_val;
    Value dist_val;
    Value colour_val;

    At_Program cx(*this);

    auto r = val.dycast<Record>();
    if (r == nullptr)
        return false;
    if (r->hasfield(is_2d_key))
        is_2d_val = r->getfield(is_2d_key, cx);
    else
        return false;
    if (r->hasfield(is_3d_key))
        is_3d_val = r->getfield(is_3d_key, cx);
    else
        return false;
    if (r->hasfield(bbox_key))
        bbox_val = r->getfield(bbox_key, cx);
    else
        return false;
    if (r->hasfield(dist_key))
        dist_val = r->getfield(dist_key, cx);
    else
        return false;
    if (r->hasfield(colour_key))
        colour_val = r->getfield(colour_key, cx);
    else
        return false;

    is_2d_ = is_2d_val.to_bool(At_Field("is_2d", cx));
    is_3d_ = is_3d_val.to_bool(At_Field("is_3d", cx));
    if (!is_2d_ && !is_3d_)
        throw Exception(cx,
            "at least one of is_2d and is_3d must be true");
    bbox_ = BBox::from_value(bbox_val, At_Field("bbox", cx));

    dist_fun_ = dist_val.dycast<Function>();
    if (dist_fun_ == nullptr)
        throw Exception(At_Field("dist", cx), "dist is not a function");
    dist_frame_ = Frame::make(
        dist_fun_->nslots_, system_, nullptr, nullptr, nullptr);

    colour_fun_ = colour_val.dycast<Function>();
    if (colour_fun_ == nullptr)
        throw Exception(At_Field("colour", cx),
            "colour is not a function");
    colour_frame_ = Frame::make(
        colour_fun_->nslots_, system_, nullptr, nullptr, nullptr);

    record_ = r;
    return true;
}

Shape_Program::Shape_Program(
    const Shape_Program& shape,
    Shared<Record> r,
    Viewed_Shape* vs)
:
    nub_(shape.nub_),
    system_(shape.system_),
    record_(r),
    viewed_shape_(vs)
{
    is_2d_ = shape.is_2d_;
    is_3d_ = shape.is_3d_;
    bbox_ = shape.bbox_;

    At_Program cx(*this);

    if (r->hasfield("dist"))
        dist_fun_ = r->getfield("dist", cx).to<Function>(cx);
    else
        throw Exception{cx, stringify(
            "bad parametric shape: call result has no 'dist' field: ", r)};

    if (r->hasfield("colour"))
        colour_fun_ = r->getfield("colour", cx).to<Function>(cx);
    else
        throw Exception{cx, stringify(
            "bad parametric shape: call result has no 'colour' field: ", r)};
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
Shape_Program::gl_dist(GL_Value arg, GL_Compiler& gl) const
{
    assert(arg.type == GL_Type::Vec(4));
    At_Program cx0(*this);
    const At_Field cx("dist", cx0);
    auto f = GL_Frame::make(0, gl, &cx, nullptr, nullptr);
    auto aref = make<GL_Data_Ref>(nullptr, arg);
    auto result = dist_fun_->gl_call_expr(*aref, nullptr, *f);
    if (result.type != GL_Type::Num())
        throw Exception(cx, stringify("dist function returns ",result.type));
    return result;
}

GL_Value
Shape_Program::gl_colour(GL_Value arg, GL_Compiler& gl) const
{
    assert(arg.type == GL_Type::Vec(4));
    At_Program cx0(*this);
    const At_Field cx("colour", cx0);
    auto f = GL_Frame::make(0, gl, &cx, nullptr, nullptr);
    auto aref = make<GL_Data_Ref>(nullptr, arg);
    auto result = colour_fun_->gl_call_expr(*aref, nullptr, *f);
    if (result.type != GL_Type::Vec(3))
        throw Exception(cx, stringify("colour function returns ",result.type));
    return result;
}

double
Shape_Program::dist(double x, double y, double z, double t)
{
    At_Program cx(*this);
    Shared<List> point = List::make({Value{x}, Value{y}, Value{z}, Value{t}});
    Value result = dist_fun_->call({point}, *dist_frame_);
    return result.to_num(cx);
}

Vec3
Shape_Program::colour(double x, double y, double z, double t)
{
    At_Program cx(*this);
    Shared<List> point = List::make({Value{x}, Value{y}, Value{z}, Value{t}});
    Value result = colour_fun_->call({point}, *colour_frame_);
    Shared<List> cval = result.to<List>(cx);
    cval->assert_size(3, cx);
    return Vec3{ cval->at(0).to_num(cx),
                 cval->at(1).to_num(cx),
                 cval->at(2).to_num(cx) };
}

}} // namespace
