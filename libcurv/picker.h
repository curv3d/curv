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
        slider
    };
    struct Config {
        Type type_;
        union {
            struct {
                double low_;
                double high_;
            } slider_;
        };
        void write(std::ostream&);
    };
    union State {
        float slider_; // not double, because ImGui sliders use float
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

// Constructed using the `slider` builtin function.
struct Slider_Picker : public Picker
{
    Slider_Picker(double lo, double hi)
    :
        Picker("slider", 1)
    {
        config_.type_ = Type::slider;
        config_.slider_.low_ = lo;
        config_.slider_.high_ = hi;
    }

    virtual Value call(Value v, Frame& f) override;
};

// This is a pseudo-value that denotes a GLSL uniform variable.
// More specifically, it's one of the parameters in a parametric shape.
// These things are only used when compiling a parametric shape to GLSL.
struct Uniform_Variable : public Reactive_Value
{
    Symbol name_;
    Uniform_Variable(Symbol name, Picker::Type ptype);
    virtual void print(std::ostream&) const override;
};

} // namespace curv
#endif // header guard
