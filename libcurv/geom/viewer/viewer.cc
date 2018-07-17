// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/viewer/viewer.h>
#include <iostream>
#include <sstream>
#include <libcurv/string.h>
#include <libcurv/geom/export_frag.h>
#include <libcurv/geom/tempfile.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>
#include <tools/fs.h>

namespace curv { namespace geom { namespace viewer {

void
Viewer::set_shape(Shape_Recognizer& shape)
{
    std::stringstream f;
    export_frag(shape, f);
    fragsrc_ = f.str();
}

void
Viewer::run()
{
    int status = Viewer::main(this);
    if (status != 0)
        throw Exception({}, "Viewer error");
}

void
Viewer::on_close()
{
}

bool Viewer::next_frame()
{
    return true;
}

void Viewer::set_frag(const std::string& fragSource)
{
    shader_.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
    shader_.load(fragSource, vertSource_, defines_, verbose_);
}

}}} // namespace
