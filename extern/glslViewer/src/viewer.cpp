// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <viewer.h>
#include <iostream>
#include <fstream>
#include <libcurv/string.h>
extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/unistd.h>
#include <sys/wait.h>
}

void
viewer_run(int argc, const char** argv)
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

void
viewer_run_frag(std::string shader)
{
    (void) shader;
}

void
viewer_spawn_frag(std::string shader)
{
    (void) shader;
}

void
viewer_export_png(std::string shader, curv::Filesystem::path png_pathname)
{
    auto fragname = curv::stringify(",curv",getpid(),".frag");
    std::ofstream f(fragname->c_str());
    f << shader;
    f.close();

    const char *argv[8];
    argv[0] = "glslViewer";
    argv[1] = "-s";
    argv[2] = "0";
    argv[3] = "--headless";
    argv[4] = "-o";
    argv[5] = png_pathname.c_str();
    argv[6] = fragname->c_str();
    argv[7] = nullptr;
    viewer_run(7, argv);
}
