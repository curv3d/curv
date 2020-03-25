// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifdef _WIN32
    #include <libcurv/win32.h>
    extern "C" {
        #include <shellapi.h>
    }
#endif

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

#ifdef _WIN32
    using editor_handle_t = HANDLE;
#else
    using editor_handle_t = pid_t;
#endif

// Check whether the given editor handle is a valid handle.
// Think of it as a non-null check with the quirks that
// the null value is represented by, say, (pid_t) -1 on *nix OSes.
//
// On Windows, not even pseudo handles [1, last paragraph] are deemed valid.
// [1]: https://devblogs.microsoft.com/oldnewthing/20040302-00/?p=40443
inline bool
is_valid_editor_handle(editor_handle_t editor_handle)
{
#ifdef _WIN32
    // The WinAPI uses two different "null values" for handles, see
    // https://devblogs.microsoft.com/oldnewthing/20040302-00/?p=40443
    return (editor_handle != NULL && editor_handle != INVALID_HANDLE_VALUE);
#else
    return (editor_handle != (pid_t) -1);
#endif
}

editor_handle_t
launch_editor(const char* editor, const char* filename)
{
    // the full OS-independent command line to the editor application invoked on filename
    // we quote the filename for cases where it might contain spaces
    const char* editor_commandline = curv::stringify(editor, " ", "\"", filename, "\"")->c_str();

#ifdef _WIN32
    STARTUPINFOW startup_info = {sizeof(startup_info)};
    PROCESS_INFORMATION proc_info = {};

    BOOL createProcessSuccess = CreateProcessW(
        // signal to specify full command line as next argument
        NULL,
        // full command line (is potentially modified by CreateProcessW, so needs to be writable!)
        &curv::str_to_wstring(editor_commandline)[0],
        // default process attributes
        NULL,
        // default thread attributes
        NULL,
        // do not inherit handles in new process
        false,
        // CREATE_NEW_CONSOLE makes non-GUI console editors (e.g. vim) work as well
        // at the same time, GUI applications (e.g. notepad) are still supported
        CREATE_NEW_CONSOLE | CREATE_UNICODE_ENVIRONMENT,
        // inherit environment
        NULL,
        // same "current directory" as us
        NULL,
        &startup_info,
        &proc_info
    );

    if (!createProcessSuccess)
    {
        DWORD error = GetLastError();
        throw curv::Exception_Base(curv::stringify("Could not launch editor: ", curv::win_strerror(error)));
    }

    // Close this handle right away as we only need proc_info.hProcess in poll_editor()
    // below
    CloseHandle(proc_info.hThread);

    return proc_info.hProcess;
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
        throw curv::Exception_Base("Cannot fork $CURV_EDITOR"); // TODO: why?
    } else {
        return pid; // PID of launched editor process
    }
#endif
}

// Check whether the editor behind editor_handle is still alive.
//
// When given invalid_editor_handle, then false is returned.
// Otherwise, we return true iff. the editor is still alive.
bool
poll_editor(editor_handle_t editor_handle)
{
    if (!is_valid_editor_handle(editor_handle))
    {
        return false;
    }

#ifdef _WIN32
    if (WaitForSingleObject(editor_handle, 0) == WAIT_TIMEOUT)
    {
        return true;
    }
    else
    {
        // The editor is no longer alive, free up handle
        CloseHandle(editor_handle);
        return false;
    }
#else
    int status;
    pid_t pid = waitpid(editor_handle, &status, WNOHANG);
    if (pid == editor_handle) {
        // TODO: print abnormal exit status
        return false;
    } else
        return true;
#endif
}

void
poll_file(
    curv::System* sys, curv::viewer::Viewer_Config* opts,
    editor_handle_t *editor_handle, const char* filename)
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
            if (editor_handle && !poll_editor(*editor_handle)) {
                // We actually started an editor, but it got now closed as signalled by poll_editor
                // => also exit Curv's livemode
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
    editor_handle_t editor_handle;
    if (editor)
    {
        editor_handle = launch_editor(editor, filename);
        if (!poll_editor(editor_handle))
            return EXIT_FAILURE;
    }

    editor_handle_t *editor_handle_ptr = editor ? &editor_handle : nullptr;

    std::thread poll_file_thread = std::thread(poll_file, &sys, &opts, editor_handle_ptr, filename);
    live_view_server.run(opts);
    if (poll_file_thread.joinable())
        poll_file_thread.join();
    return EXIT_SUCCESS;
}
