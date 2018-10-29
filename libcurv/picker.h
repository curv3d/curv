// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_PICKER_H
#define LIBCURV_PICKER_H

#include <libcurv/function.h>

namespace curv {

// A Picker is a predicate function that carries additional metadata,
// describing the type and range of a graphical value picker.
// For now, I will use RTTI to distinguish subtypes of picker.
struct Picker : public Function
{
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
    double lo_;
    double hi_;

    Slider_Picker(double lo, double hi)
    :
        Picker("slider", 1),
        lo_(lo), hi_(hi)
    {}

    virtual Value call(Value v, Frame& f) { return v.is_num(); }
};

} // namespace curv
#endif // header guard

