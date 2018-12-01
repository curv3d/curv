// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/picker.h>
#include <libcurv/exception.h>
#include <libcurv/die.h>
#include <libcurv/math.h>

namespace curv {

void
Picker::Config::write(std::ostream& out)
{
    switch (type_) {
    case Type::slider:
        out << "slider(" << slider_.low_ << "," << slider_.high_ << ")";
        break;
    case Type::checkbox:
        out << "checkbox";
        break;
    case Type::colour_picker:
        out << "colour_picker";
        break;
    default:
        out << "bad picker config type " << int(type_);
        break;
    }
}

void
Picker::State::write(std::ostream& out, GL_Type gltype)
{
    switch (gltype) {
    case GL_Type::Bool:
        out << bool_;
        break;
    case GL_Type::Num:
        out << num_;
        break;
    case GL_Type::Vec3:
        out << "[" << vec3_[0] << "," << vec3_[1] << "," << vec3_[2] << "]";
        break;
    default:
        out << "bad picker value type " << gltype;
        break;
    }
}

Picker::State::State(GL_Type gltype, Value val, const Context& cx)
{
    switch (gltype) {
    case GL_Type::Bool:
        bool_ = val.to_bool(cx);
        break;
    case GL_Type::Num:
        num_ = val.to_num(cx);
        break;
    case GL_Type::Vec3:
      {
        auto v = val.to<List>(cx);
        v->assert_size(3, cx);
        vec3_[0] = v->at(0).to_num(cx);
        vec3_[1] = v->at(1).to_num(cx);
        vec3_[2] = v->at(2).to_num(cx);
        break;
      }
    default:
        throw Exception{cx, stringify("bad picker value type ", gltype)};
    }
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

Value Checkbox_Picker::call(Value v, Frame& f)
{
    return isbool(v);
}

Value Colour_Picker::call(Value v, Frame& f)
{
    return isvec3(v);
}

} // namespace curv
