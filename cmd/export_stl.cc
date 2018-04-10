// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <iostream>

#include "export.h"
#include <curv/shape.h>
#include <curv/exception.h>

void export_stl(curv::Value value,
    curv::System& sys, const curv::Context& cx, const Export_Params& params,
    std::ostream& out)
{
    (void)out;
    curv::Shape_Recognizer shape(cx, sys);
    if (!shape.recognize(value))
        throw curv::Exception(cx, "not a shape");

    for (auto p : params) {
        std::cerr << p.first << "=" << p.second << "\n";
    }
    throw curv::Exception(cx, "STL export not supported yet");
}
