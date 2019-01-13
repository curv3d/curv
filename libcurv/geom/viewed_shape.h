// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_VIEWED_SHAPE_H
#define LIBCURV_GEOM_VIEWED_SHAPE_H

#include <libcurv/picker.h>
#include <libcurv/geom/frag.h>
#include <libcurv/geom/shape.h>
#include <tsl/ordered_map.h>

namespace curv {
namespace geom {

// Viewed_Shape is a representation of a Shape that is directly usable by
// the Viewer. It encodes the frag program, uniform variables, and pickers.
// It is self contained, containing no references to data owned by the
// evaluator thread.

// Contains an array of Parameters. A Parameter comprises:
//   name
//   GLuint uniform_variable_id, maybe
//   Picker: picker type & picker config
//   current value, part of the picker state (type dependent)

// How is the picker config and the picker state represented?
// Requirements:
// * The picker config is stored in Curv picker values, and is deep-copied into
//   the Viewed_Shape: rep should support this.
// * How is dynamically allocated picker state represented? Eg, string picker.
//   Note ImGui InputText works with fixed size C arrays, not compatible with
//   std::string.
// Alternatives:
// * Pure C, a union. Allocated picker state is stored as C pointers which
//   are freed by Parameter destructor.
// * C++17 std::variant. Or Boost.Variant.
// * Abstract class with subclass for each picker type.
//   One hierarchy for the config, and one for the state.

struct Viewed_Shape
{
    std::string frag_;

    // If the shape is parametric, there will be one or more parameters.
    struct Parameter
    {
        Picker::Config pconfig_;
        Picker::State pstate_;
        Picker::State default_state_;
    };
    tsl::ordered_map<std::string, Parameter> param_{};

    // This creates an empty Viewed_Shape (contains no shape).
    Viewed_Shape() {};

    // This creates a non-empty Viewed_Shape (contains a viewable shape).
    Viewed_Shape(const Shape_Program& shape, const Frag_Export& opts);

    bool empty() const { return frag_.empty(); }

    // Serialize as a sequence of JSON object fields,
    // without an enclosing '{...}'.
    void write_json(std::ostream&) const;
};

}} // namespace
#endif // header guard
