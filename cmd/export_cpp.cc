// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include "export.h"
#include "compiled_shape.h"
#include <fstream>
#include <curv/exception.h>
#include <curv/shape.h>

void export_cpp(curv::Value value,
    curv::System& sys, const curv::Context& cx, const Export_Params&,
    std::ostream& out)
{
    curv::Shape_Recognizer shape(cx, sys);
    if (!shape.recognize(value))
        throw curv::Exception(cx, "not a shape");
    shape_to_cpp(shape, out);
}
