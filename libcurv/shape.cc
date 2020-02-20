// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/shape.h>

#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/frame.h>
#include <libcurv/function.h>
#include <libcurv/program.h>
#include <libcurv/render.h>
#include <libcurv/sc_context.h>

#include <cmath>

namespace curv {

Shape_Program::Shape_Program(
    Program& prog)
:
    system_(prog.system()),
    nub_(nub_phrase(prog.phrase_))
{
    // mark initial state (no shape has been recognized yet)
    is_2d_ = false;
    is_3d_ = false;
}

Location Shape_Program::location() const
{
    return nub_->location();
}

Vec3 value_to_vec3(Value val, const Context& cx)
{
    auto list = val.to<List>(cx);
    list->assert_size(3, cx);
    Vec3 v;
    v.x = list->at(0).to_num(At_Index(0, cx));
    v.y = list->at(1).to_num(At_Index(1, cx));
    v.z = list->at(2).to_num(At_Index(2, cx));
    return v;
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
Shape_Program::recognize(Value val, Render_Opts* opts)
{
    static Symbol_Ref is_2d_key = make_symbol("is_2d");
    static Symbol_Ref is_3d_key = make_symbol("is_3d");
    static Symbol_Ref bbox_key = make_symbol("bbox");
    static Symbol_Ref dist_key = make_symbol("dist");
    static Symbol_Ref colour_key = make_symbol("colour");
    static Symbol_Ref render_key = make_symbol("render");

    At_Program cx(*this);

    auto r = val.dycast<Record>();
    if (r == nullptr) return false;
    Value is_2d_val = r->find_field(is_2d_key, cx);
    if (is_2d_val.is_missing()) return false;
    Value is_3d_val = r->find_field(is_3d_key, cx);
    if (is_3d_val.is_missing()) return false;
    Value bbox_val = r->find_field(bbox_key, cx);
    if (bbox_val.is_missing()) return false;
    Value dist_val = r->find_field(dist_key, cx);
    if (dist_val.is_missing()) return false;
    Value colour_val = r->find_field(colour_key, cx);
    if (colour_val.is_missing()) return false;

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

    Value render_val = r->find_field(render_key, cx);
    if (!render_val.is_missing()) {
        At_Field rcx("render", cx);
        auto render = render_val.to<Record>(rcx);
        if (opts != nullptr) {
            // update *opts from fields in the render record
            opts->update_from_record(*render, rcx);
        }
    }

    record_ = r;
    return true;
}

Shape_Program::Shape_Program(
    const Shape_Program& shape,
    Shared<Record> r,
    Viewed_Shape* vs)
:
    system_(shape.system_),
    nub_(shape.nub_),
    record_(r),
    viewed_shape_(vs)
{
    static Symbol_Ref dist_key = make_symbol("dist");
    static Symbol_Ref colour_key = make_symbol("colour");

    is_2d_ = shape.is_2d_;
    is_3d_ = shape.is_3d_;
    bbox_ = shape.bbox_;

    At_Program cx(*this);

    if (r->hasfield(dist_key))
        dist_fun_ = r->getfield(dist_key, cx).to<Function>(cx);
    else
        throw Exception{cx, stringify(
            "bad parametric shape: call result has no 'dist' field: ", r)};

    if (r->hasfield(colour_key))
        colour_fun_ = r->getfield(colour_key, cx).to<Function>(cx);
    else
        throw Exception{cx, stringify(
            "bad parametric shape: call result has no 'colour' field: ", r)};
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

} // namespace
