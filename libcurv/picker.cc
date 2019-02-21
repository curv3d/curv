// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/picker.h>

#include <libcurv/context.h>
#include <libcurv/die.h>
#include <libcurv/exception.h>
#include <libcurv/math.h>

#include <climits>

namespace curv {

Picker::Config::Config(Value val, const Context& cx)
{
    auto config_v = value_to_variant(val, cx);
    if (config_v.first == "slider") {
        type_ = Picker::Type::slider;
        gltype_ = GL_Type::Num();
        At_Field list_cx("slider", cx);
        auto list = config_v.second.to<List>(list_cx);
        list->assert_size(2, list_cx);
        slider_.low_ = list->at(0).to_num(At_Index(0, list_cx));
        slider_.high_ = list->at(1).to_num(At_Index(1, list_cx));
    } else if (config_v.first == "int_slider") {
        type_ = Picker::Type::int_slider;
        gltype_ = GL_Type::Num();
        At_Field list_cx("int_slider", cx);
        auto list = config_v.second.to<List>(list_cx);
        list->assert_size(2, list_cx);
        int_slider_.low_ =
            list->at(0).to_int(INT_MIN,INT_MAX,At_Index(0, list_cx));
        int_slider_.high_
            = list->at(1).to_int(INT_MIN,INT_MAX,At_Index(1, list_cx));
    } else if (config_v.first == "scale_picker") {
        type_ = Picker::Type::scale_picker;
        gltype_ = GL_Type::Num();
    } else if (config_v.first == "checkbox") {
        type_ = Picker::Type::checkbox;
        gltype_ = GL_Type::Bool();
    } else if (config_v.first == "colour_picker") {
        type_ = Picker::Type::colour_picker;
        gltype_ = GL_Type::Vec(3);
    } else {
        throw Exception(cx, "not a picker descriptor");
    }
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
        out << "{\"scale_picker\":null}";
        return;
    case Type::checkbox:
        out << "{\"checkbox\":null}";
        return;
    case Type::colour_picker:
        out << "{\"colour_picker\":null}";
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
        out << "{scale_picker:null}";
        return;
    case Type::checkbox:
        out << "{checkbox:null}";
        return;
    case Type::colour_picker:
        out << "{colour_picker:null}";
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

Uniform_Variable::Uniform_Variable(Symbol name, std::string id, GL_Type gltype)
:
    Reactive_Value(Ref_Value::sty_uniform_variable, gltype),
    name_(std::move(name)),
    identifier_(std::move(id))
{
}

void Uniform_Variable::print(std::ostream& out) const
{
    out << "<uniform " << name_ << ">";
}

} // namespace curv
