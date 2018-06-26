// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/viewer/viewer.h>
#include <glslviewer.h>
#include <iostream>
#include <fstream>
#include <libcurv/string.h>
#include <libcurv/geom/export_frag.h>
#include <libcurv/geom/tempfile.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>
extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include <sys/wait.h>
#include <signal.h>
}

namespace curv { namespace geom { namespace viewer {

pid_t viewer_pid = pid_t(-1);

void
poll_viewer()
{
    if (viewer_pid != pid_t(-1)) {
        int status;
        pid_t pid = waitpid(viewer_pid, &status, WNOHANG);
        if (pid == viewer_pid) {
            if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
                ;
            else {
                std::cerr << "Viewer process abnormal exit: " << status << "\n";
            }
            viewer_pid = pid_t(-1);
        }
    }
}

void
launch_viewer(boost::filesystem::path filename)
{
    poll_viewer();
    if (viewer_pid == pid_t(-1)) {
        pid_t pid = fork();
        if (pid == 0) {
            // in child process
            const char* argv[3];
            argv[0] = "curv";
            argv[1] = filename.c_str();
            argv[2] = nullptr;
            exit(glslviewer_main(2, argv));
        } else if (pid == pid_t(-1)) {
            std::cerr << "can't fork Viewer process: "
                      << strerror(errno) << "\n";
        } else {
            viewer_pid = pid;
        }
    }
}

void
open_viewer(Shape_Recognizer& shape)
{
    auto filename = make_tempfile(".frag");
    std::ofstream f(filename.c_str());
    export_frag(shape, f);
    f.close();
    launch_viewer(filename);
}

void
close_viewer()
{
    if (viewer_pid != (pid_t)(-1))
        kill(viewer_pid, SIGTERM);
}

void
Viewer::set_shape(Shape_Recognizer& shape)
{
    fragname_ = make_tempfile(".frag");
    std::ofstream f(fragname_.c_str());
    export_frag(shape, f);
    f.close();
}

void
Viewer::run()
{
    const char* argv[3];
    argv[0] = "curv";
    argv[1] = fragname_.c_str();
    argv[2] = nullptr;
    int status = glslviewer_main(2, argv);
    if (status != 0)
        throw Exception({}, "Viewer error");
}

}}} // namespace
