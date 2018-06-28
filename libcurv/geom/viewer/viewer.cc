// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/viewer/viewer.h>
#include <iostream>
#include <fstream>
#include <libcurv/string.h>
#include <libcurv/geom/export_frag.h>
#include <libcurv/geom/tempfile.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>
/*
extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <signal.h>
}
*/

namespace curv { namespace geom { namespace viewer {

Viewer::~Viewer()
{
    if (thread_.joinable()) {
        { std::lock_guard<std::mutex> lock(mutex_);
          request_ = Request::k_close;
        }
        thread_.join();
    }
}

void
Viewer::set_shape(Shape_Recognizer& shape)
{
    // TODO: create the frag shader somewhere the Viewer thread can't see it,
    // then transfer ownership of the shader atomically while holding mutex_.
    if (fragname_.empty())
        fragname_ = make_tempfile(".frag");
    std::ofstream f(fragname_.c_str());
    export_frag(shape, f);
    f.close();
    {
        std::lock_guard<std::mutex> lock(mutex_);
        request_ = Request::k_new_shape;
    }
}

void
Viewer::run()
{
    int status = Viewer::main(this);
    if (status != 0)
        throw Exception({}, "Viewer error");
}

void
Viewer::open()
{
    // If the Viewer is open on entry, then it could be in the process of
    // closing due to the user clicking the close button, and the window
    // could close milliseconds after we verified that it was open.
    // In that case, open() doesn't do anything, by design.
    if (!is_open_) {
        if (thread_.joinable())
            thread_.join();
        std::lock_guard<std::mutex> lock(mutex_);
        is_open_ = true;
        request_ = Request::k_none;
        std::thread new_thread(Viewer::main, this);
        swap(thread_, new_thread);
    }
}

void
Viewer::close()
{
    std::lock_guard<std::mutex> lock(mutex_);
    request_ = Request::k_close;
}

}}} // namespace
