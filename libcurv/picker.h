// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_PICKER_H
#define LIBCURV_PICKER_H

#include <libcurv/function.h>
#include <libcurv/reactive.h>
#include <libcurv/symbol.h>

namespace curv {

// A Picker is a predicate function that carries additional metadata,
// describing the type and range of a graphical value picker.
struct Picker : public Function
{
    // Type, Config and State describe a graphical value picker.
    enum class Type {
        num_slider,
        int_slider,
        checkbox,
        colour_picker
    };
    struct Config {
        Type type_;
        GL_Type gltype_;
        GL_Subtype glsubtype_{GL_Subtype::None};
        union {
            struct {
                double low_;
                double high_;
            } num_slider_;
            struct {
                int low_;
                int high_;
            } int_slider_;
        };
        void write(std::ostream&);
    };
    union State {
        bool bool_;
        int int_;
        float num_; // not double, because ImGui sliders use float
        float vec3_[3];
        State(Type, Value, const Context&);
        void write(std::ostream&, Type);
    };
    
    Config config_;

    Picker(const char* name, int argpos)
    :
        Function(0, name)
    {
        argpos_ = argpos;
    }

    // Predicates are total functions over the set of values.
    // There are no pattern match failures.
    virtual Value try_call(Value v, Frame& f) override { return call(v,f); }
};

// Constructed using the `num_slider` builtin function.
struct Num_Slider_Picker : public Picker
{
    Num_Slider_Picker(double lo, double hi)
    :
        Picker("num_slider", 1)
    {
        config_.type_ = Type::num_slider;
        config_.gltype_ = GL_Type::Num;
        config_.num_slider_.low_ = lo;
        config_.num_slider_.high_ = hi;
    }

    virtual Value call(Value v, Frame& f) override;
};

// Constructed using the `int_slider` builtin function.
struct Int_Slider_Picker : public Picker
{
    Int_Slider_Picker(int lo, int hi)
    :
        Picker("int_slider", 1)
    {
        config_.type_ = Type::int_slider;
        config_.gltype_ = GL_Type::Num;
        config_.glsubtype_ = GL_Subtype::Int;
        config_.int_slider_.low_ = lo;
        config_.int_slider_.high_ = hi;
    }

    virtual Value call(Value v, Frame& f) override;
};

// The `checkbox` builtin function.
struct Checkbox_Picker : public Picker
{
    Checkbox_Picker()
    :
        Picker("checkbox", 0)
    {
        config_.type_ = Type::checkbox;
        config_.gltype_ = GL_Type::Bool;
    }

    virtual Value call(Value v, Frame& f) override;
};

// The `colour_picker` builtin function.
struct Colour_Picker : public Picker
{
    Colour_Picker()
    :
        Picker("colour_picker", 0)
    {
        config_.type_ = Type::colour_picker;
        config_.gltype_ = GL_Type::Vec3;
    }

    virtual Value call(Value v, Frame& f) override;
};

// This is a pseudo-value that denotes a GLSL uniform variable.
// More specifically, it's one of the parameters in a parametric shape.
// These things are only used when compiling a parametric shape to GLSL.
struct Uniform_Variable : public Reactive_Value
{
    Symbol name_;
    Uniform_Variable(Symbol name, GL_Type, GL_Subtype);
    virtual void print(std::ostream&) const override;
};

} // namespace curv
#endif // header guard
