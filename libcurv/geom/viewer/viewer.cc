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
#include <tools/fs.h>

namespace curv { namespace geom { namespace viewer {

Threaded_Viewer::~Threaded_Viewer()
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
}

void
Threaded_Viewer::set_shape(Shape_Recognizer& shape)
{
    Viewer::set_shape(shape);
    std::lock_guard<std::mutex> lock(mutex_);
    request_ = Request::k_new_shape;
}

void
Viewer::run()
{
    int status = Viewer::main(this);
    if (status != 0)
        throw Exception({}, "Viewer error");
}

void
Threaded_Viewer::open()
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
Threaded_Viewer::close()
{
    std::lock_guard<std::mutex> lock(mutex_);
    request_ = Request::k_close;
}

void
Viewer::on_close()
{
}

void
Threaded_Viewer::on_close()
{
    std::lock_guard<std::mutex> lock(mutex_);
    // Only becomes false once the thread is past the point of accessing
    // any Viewer state.
    is_open_ = false;
}

bool Viewer::next_frame()
{
    return true;
}

bool Threaded_Viewer::next_frame()
{
    if (request_ != Request::k_none) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (request_ == Request::k_close) {
            request_ = Request::k_none;
            return false;
        }
        if (request_ == Request::k_new_shape) {
            std::string fragSource = "";
            std::vector<std::string> include_folders;
            if (loadFromPath(fragname_.c_str(), &fragSource, include_folders))
                set_frag(fragSource);
            request_ = Request::k_none;
        }
    }
    return true;
}

void Viewer::set_frag(const std::string& fragSource)
{
    shader_.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
    shader_.load(fragSource, vertSource_, defines_, verbose_);
}

}}} // namespace
