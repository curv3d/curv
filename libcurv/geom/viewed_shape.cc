// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/viewed_shape.h>

namespace curv {
namespace geom {

Viewed_Shape::Viewed_Shape(const Shape_Program& shape, const Frag_Export& opts)
{
    // Recognize a parametric shape S, which contains call & parameter fields.
    // If found:
    // * Extract the parameters.
    //   Each parameter has: name, value, optional picker.
    //   The parameters are stored in the Viewed_Shape.
    // * Create a parameter record A (a Value). Parameters that have a picker
    //   get a Reactive Value as their argument. Non-picker parameters get a
    //   proper value taken from S.parameter.
    // * Evaluate S.call(A).
    // * The result must be a shape record S1 (otherwise error).
    // * GL compile S1.dist and S1.colour into GLSL. References to reactive
    //   values are converted to uniform variable references. Note that S1.bbox
    //   etc are ignored. (glsl_function_export)
    // If not found:
    // * GL compile S.dist and S.colour into GLSL. (glsl_function_export)

    // Recognize a parametric shape, create ptable.
    // If found, create S1. Store ptable ptr in S1.
    //   Call export_frag(S1), which calls glsl_function_export(S1),
    //   which outputs uniform variable references from ptable.
    // Else, call export_frag(S), the boring case.

    // Viewed_Shape data structures:
    // * A set of reactive values, used by the GL compiler. These are not
    //   stored in the Viewed_Shape itself, because it is self contained,
    //   doesn't reference objects shared with other threads.
    // * A parameter table, used by the Viewer in the OpenGL main loop.
    //   This is stored in the Viewed_Shape.
    //   It's an array, not a map. Each element:
    //   * GLSL variable name, referenced by GLSL frag shader, passed to
    //     glGetUniformLocation to get GLint id used to set the value.
    //   * picker config
    //     * type, an enum I guess
    //     * optional config values, like (lo,hi) for a slider
    //   * current picker value, value type depends on picker type
    //   * A picker descriptor is either a variant record (a struct with an
    //     enum type and a union of type dependent values), or it is a
    //     unique_ptr to a picker subclass instance, and RTTI is used.
    //   If I use IMGUI, then I iterate over the parameter table and render
    //   each picker.

    std::stringstream frag;
    export_frag(shape, opts, frag);
    frag_ = frag.str();
}

}} // namespace
