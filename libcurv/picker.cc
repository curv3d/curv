// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/picker.h>
#include <libcurv/exception.h>
#include <libcurv/die.h>
#include <libcurv/math.h>
#include <climits>

namespace curv {

void
Picker::Config::write(std::ostream& out) const
{
    switch (type_) {
    case Type::slider:
        out << "slider(" << slider_.low_
            << "," << slider_.high_ << ")";
        return;
    case Type::int_slider:
        out << "int_slider(" << int_slider_.low_
            << "," << int_slider_.high_ << ")";
        return;
    case Type::scale_picker:
        out << "scale_picker";
        return;
    case Type::checkbox:
        out << "checkbox";
        return;
    case Type::colour_picker:
        out << "colour_picker";
        return;
    }
    out << "bad picker config type " << int(type_);
}

void
Picker::Config::write_json(std::ostream& out) const
{
    switch (type_) {
    case Type::slider:
        out << "{\"slider\":[" << slider_.low_
            << "," << slider_.high_ << "]}";
        return;
    case Type::int_slider:
        out << "{\"int_slider\":[" << int_slider_.low_
            << "," << int_slider_.high_ << "]}";
        return;
    case Type::scale_picker:
        out << "{\"scale_picker\":true}";
        return;
    case Type::checkbox:
        out << "{\"checkbox\":true}";
        return;
    case Type::colour_picker:
        out << "{\"colour_picker\":true}";
        return;
    }
    out << "\"bad picker config type " << int(type_) << "\"";
}

void
Picker::Config::write_curv(std::ostream& out) const
{
    switch (type_) {
    case Type::slider:
        out << "{slider:[" << slider_.low_
            << "," << slider_.high_ << "]}";
        return;
    case Type::int_slider:
        out << "{int_slider:[" << int_slider_.low_
            << "," << int_slider_.high_ << "]}";
        return;
    case Type::scale_picker:
        out << "{scale_picker:true}";
        return;
    case Type::checkbox:
        out << "{checkbox:true}";
        return;
    case Type::colour_picker:
        out << "{colour_picker:true}";
        return;
    }
    out << "\"bad picker config type " << int(type_) << "\"";
}

void
Picker::State::write(std::ostream& out, Picker::Type ptype) const
{
    switch (ptype) {
    case Type::checkbox:
        out << (bool_ ? "true" : "false");
        return;
    case Type::int_slider:
        out << int_;
        return;
    case Type::slider:
    case Type::scale_picker:
        out << num_;
        return;
    case Type::colour_picker:
        out << "[" << vec3_[0] << "," << vec3_[1] << "," << vec3_[2] << "]";
        return;
    }
    out << "bad picker value type " << int(ptype);
}

void
Picker::State::write_json(std::ostream& out, Picker::Type ptype) const
{
    switch (ptype) {
    case Type::checkbox:
        out << (bool_ ? "true" : "false");
        return;
    case Type::int_slider:
        out << int_;
        return;
    case Type::slider:
    case Type::scale_picker:
        out << num_;
        return;
    case Type::colour_picker:
        out << "[" << vec3_[0] << "," << vec3_[1] << "," << vec3_[2] << "]";
        return;
    }
    out << "\"bad picker value type " << int(ptype) << "\"";
}

void
Picker::State::write_curv(std::ostream& out, Picker::Type ptype) const
{
    switch (ptype) {
    case Type::checkbox:
        out << (bool_ ? "true" : "false");
        return;
    case Type::int_slider:
        out << int_;
        return;
    case Type::slider:
    case Type::scale_picker:
        out << num_;
        return;
    case Type::colour_picker:
        out << "[" << vec3_[0] << "," << vec3_[1] << "," << vec3_[2] << "]";
        return;
    }
    out << "\"bad picker value type " << int(ptype) << "\"";
}

Picker::State::State(Picker::Type ptype, Value val, const Context& cx)
{
    // TODO: enforce range constraints. Pass a Picker::Config instead.
    switch (ptype) {
    case Type::checkbox:
        bool_ = val.to_bool(cx);
        return;
    case Type::int_slider:
        int_ = val.to_int(INT_MIN, INT_MAX, cx);
        return;
    case Type::slider:
    case Type::scale_picker:
        num_ = val.to_num(cx);
        return;
    case Type::colour_picker:
      {
        auto v = val.to<List>(cx);
        v->assert_size(3, cx);
        vec3_[0] = v->at(0).to_num(cx);
        vec3_[1] = v->at(1).to_num(cx);
        vec3_[2] = v->at(2).to_num(cx);
        return;
      }
    }
    throw Exception{cx, stringify("bad picker type ", int(ptype))};
}

Uniform_Variable::Uniform_Variable(Symbol name, GL_Type gltype)
:
    Reactive_Value(Ref_Value::sty_uniform_variable, gltype),
    name_(std::move(name))
{
}

void Uniform_Variable::print(std::ostream& out) const
{
    out << "<uniform " << name_ << ">";
}

Value Slider_Picker::call(Value v, Frame& f)
{
    return isnum(v);
}

Value Int_Slider_Picker::call(Value v, Frame& f)
{
    return isnum(v);
}

Value Scale_Picker::call(Value v, Frame& f)
{
    return isnum(v);
}

Value Checkbox_Picker::call(Value v, Frame& f)
{
    return isbool(v);
}

Value Colour_Picker::call(Value v, Frame& f)
{
    return isvec3(v);
}

} // namespace curv
