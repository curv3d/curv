// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#include <iostream>

#include <curv/value.h>
#include <curv/system.h>
#include <curv/context.h>
#include <curv/shape.h>
#include <curv/exception.h>

void export_stl(curv::Value value,
    curv::System& sys, const curv::Context& cx, std::ostream& out)
{
    (void)out;
    curv::Shape_Recognizer shape(cx);
    if (!shape.recognize(value))
        throw curv::Exception(cx, "not a shape");

    throw curv::Exception(cx, "STL export not supported yet");
}
