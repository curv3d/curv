// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <viewer.h>
#include <iostream>
#include <fstream>
#include <libcurv/string.h>
#include <libvgeom/export_png.h>
#include <libvgeom/export_frag.h>
#include <libvgeom/tempfile.h>
extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include <sys/wait.h>
}

namespace vgeom {

void
run_glslViewer(int argc, const char** argv)
{
    pid_t pid = fork();
    if (pid == 0) {
        // in child process
        int fd = open("/dev/null", O_WRONLY);
        dup2(fd, 1); // TODO: suppress unwanted messages written to stdout
        exit(viewer_main(argc, argv));
    } else if (pid == pid_t(-1)) {
        std::cerr << "can't fork Viewer process: " << strerror(errno) << "\n";
    } else {
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
            ;
        else {
            std::cerr << "Viewer process abnormal exit: " << status << "\n";
        }
    }
}

#if 0
void
viewer_run_shape(curv::Shape_Recognizer& shape)
{
    auto filename = make_tempfile(".frag");
,,,
        std::ofstream f(filename.c_str());
        curv::gl_compile(shape, f, cx);
        f.close();
        if (block) {
            auto cmd = curv::stringify("glslViewer ",filename.c_str(),
                block ? "" : "&");
            system(cmd->c_str());
            unlink(filename.c_str());
        } else {
            launch_viewer(filename);
        }
    auto fragname = curv::stringify(",curv",getpid(),".frag");
    std::ofstream f(fragname->c_str());
    f << shader;
    f.close();

    const char *argv[3];
    argv[0] = "glslViewer";
    argv[1] = fragname->c_str();
    argv[2] = nullptr;
    viewer_run(2, argv);
}

void
viewer_spawn_frag(std::string shader)
{
    (void) shader;
}
#endif

void
export_png(Shape_Recognizer& shape, curv::Filesystem::path png_pathname)
{
    auto fragname = make_tempfile(".frag");
    std::ofstream f(fragname.c_str());
    export_frag(shape, f);
    f.close();

    const char *argv[8];
    argv[0] = "glslViewer";
    argv[1] = "-s";
    argv[2] = "0";
    argv[3] = "--headless";
    argv[4] = "-o";
    argv[5] = png_pathname.c_str();
    argv[6] = fragname.c_str();
    argv[7] = nullptr;
    run_glslViewer(7, argv);
}

} // namespace
