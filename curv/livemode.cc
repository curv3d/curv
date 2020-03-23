// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

extern "C" {
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#ifndef _WIN32
    #include <sys/wait.h>
#endif
}
#include <iostream>
#include <fstream>
#include <thread>

#include "shapes.h"
#include "view_server.h"
#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <libcurv/program.h>
#include <libcurv/source.h>
#include <libcurv/system.h>
#include <libcurv/shape.h>

View_Server live_view_server;

pid_t editor_pid = pid_t(-1);

void
launch_editor(const char* editor, const char* filename)
{
#ifdef _WIN32
    throw curv::Exception_Base("launch_editor called, but unsupported on Windows");
#else
    pid_t pid = fork();
    if (pid == 0) {
        // in child process
        int r =
            execl("/bin/sh", "sh", "-c", editor_commandline, (char*)0);
        std::cerr << "can't exec $CURV_EDITOR\n"; // TODO: why?
        (void) r; // TODO
        exit(1);
    } else if (pid == pid_t(-1)) {
        std::cerr << "can't fork $CURV_EDITOR\n"; // TODO: why?
    } else {
        editor_pid = pid;
    }
#endif
}

bool
poll_editor()
{
#ifdef _WIN32
    // fork not available under MinGW for building on Windows
    throw curv::Exception_Base("poll_editor called, but unsupported on Windows");
#else
    if (editor_pid == pid_t(-1))
        return false;
    else {
        int status;
        pid_t pid = waitpid(editor_pid, &status, WNOHANG);
        if (pid == editor_pid) {
            // TODO: print abnormal exit status
            editor_pid = pid_t(-1);
            return false;
        } else
            return true;
    }
    return false;
#endif
}

void
poll_file(
    curv::System* sys, curv::viewer::Viewer_Config* opts,
    const char* editor, const char* filename)
{
    for (;;) {
        struct stat st;
        if (stat(filename, &st) != 0) {
            // file doesn't exist.
            memset((void*)&st, 0, sizeof(st));
        } else {
            // evaluate file.
            try {
                auto file = curv::make<curv::File_Source>(
                    curv::make_string(filename), curv::At_System{*sys});
                curv::Program prog{std::move(file), *sys};
                prog.compile();
                auto value = prog.eval();
                curv::GPU_Program gprog{prog};
                if (gprog.recognize(value, *opts)) {
                    print_shape(gprog);
                    live_view_server.display_shape(std::move(gprog.vshape_));
                } else {
                    std::cout << value << "\n";
                }
            } catch (std::exception& e) {
                sys->error(e);
            }
        }
        // Wait for file to change or editor to quit.
        for (;;) {
            usleep(500'000);
            if (editor && !poll_editor()) {
                live_view_server.exit();
                return;
            }
            struct stat st2;
            if (stat(filename, &st2) != 0)
                memset((void*)&st2, 0, sizeof(st));
            if (st.st_mtime != st2.st_mtime)
                break;
        }
    }
}

int
live_mode(curv::System& sys, const char* editor, const char* filename,
    curv::viewer::Viewer_Config& opts)
{
    if (editor) {
        launch_editor(editor, filename);
        if (!poll_editor())
            return EXIT_FAILURE;
    }
    std::thread poll_file_thread{poll_file, &sys, &opts, editor, filename};
    live_view_server.run(opts);
    if (poll_file_thread.joinable())
        poll_file_thread.join();
    return EXIT_SUCCESS;
}
