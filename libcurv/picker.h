// Copyright 2016-2020 Doug Moen
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
struct Picker
{
    // Type, Config and State describe a graphical value picker.
    enum class Type {
        slider,
        int_slider,
        scale_picker,
        checkbox,
        colour_picker
    };
    struct Config {
        Type type_;
        SC_Type sctype_;
        union {
            struct {
                double low_;
                double high_;
            } slider_;
            struct {
                int low_;
                int high_;
            } int_slider_;
        };
        Config() {}
        Config(Value, const Context&);
        void write_json(std::ostream&) const;
        void write_curv(std::ostream&) const;
    };
    union State {
        bool bool_;
        int int_;
        float num_; // not double, because ImGui sliders use float
        float vec3_[3];
        State(Type, Value, const Context&);
        void write(std::ostream&, Type) const;
        void write_json(std::ostream&, Type) const;
        void write_curv(std::ostream&, Type) const;
    };
};

// This is a pseudo-value that denotes a GLSL uniform variable.
// More specifically, it's one of the parameters in a parametric shape.
// These things are only used when compiling a parametric shape to GLSL.
struct Uniform_Variable : public Reactive_Value
{
    Symbol_Ref name_;
    std::string identifier_;
    Shared<const Phrase> namephrase_;
    Uniform_Variable(Symbol_Ref name, std::string id, SC_Type,
        Shared<const Phrase> namephrase);
    virtual void print_repr(std::ostream&) const override;
    virtual Shared<Operation> expr() const override;
};

} // namespace curv
#endif // header guard
