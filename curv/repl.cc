// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

//----------------------------------------------------------------------------
// The REPL is the Read-Eval-Print Loop. It's the interactive command line
// shell that lets you type Curv expressions, actions and definitions.
//
// It has two threads: a Repl thread that runs the REPL, and a Viewer thread
// that displays shapes in a Viewer window, and runs the OpenGL frame loop.
// The Viewer thread is a "worker" thread that receives messages from
// the REPL thread telling it to display a new shape or exit.
//
// On macOS, windows can only be run in the main thread, so the Viewer thread
// must be the main thread, and we must spawn a new thread to run the REPL.
// This is an inversion of the usual practice of running workers in spawned
// threads.

extern "C" {
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <unistd.h>
}
#include <iostream>
#include <fstream>
#include <thread>
#include <condition_variable>
#include <functional>

#include <replxx.hxx>

#include "shapes.h"
#include "view_server.h"

#include <libcurv/ansi_colour.h>
#include <libcurv/context.h>
#include <libcurv/program.h>
#include <libcurv/source.h>
#include <libcurv/system.h>

#include <libcurv/geom/shape.h>
#include <libcurv/geom/viewer/viewer.h>

View_Server view_server;

bool was_interrupted = false;

void interrupt_handler(int)
{
    was_interrupted = true;
}

void repl(curv::System* sys)
{
    // Catch keyboard interrupts, and set was_interrupted = true.
    // TODO: This will be used to interrupt the evaluator.
    struct sigaction interrupt_action;
    memset((void*)&interrupt_action, 0, sizeof(interrupt_action));
    interrupt_action.sa_handler = interrupt_handler;
    sigaction(SIGINT, &interrupt_action, nullptr);

    // top level definitions, extended by typing 'id = expr'
    curv::Namespace names = sys->std_namespace();

    replxx::Replxx rx;

    for (;;) {
        was_interrupted = false;
        const char* line = rx.input(AC_PROMPT "curv> " AC_RESET);
        if (line == nullptr) {
            if (errno == EAGAIN) continue;
            std::cout << "\n";
            break;
        }
        if (line[0] != '\0')
            rx.history_add(line);

        try {
            auto source = curv::make<curv::String_Source>("", line);
            curv::Program prog{std::move(source), *sys};
            prog.compile(&names);
            auto den = prog.denotes();
            if (den.first) {
                for (auto f : *den.first)
                    names[f.first] = curv::make<curv::Builtin_Value>(f.second);
            }
            if (den.second) {
                bool is_shape = false;
                if (den.second->size() == 1) {
                    static curv::Symbol lastval_key = "_";
                    names[lastval_key] =
                        curv::make<curv::Builtin_Value>(den.second->front());
                    curv::geom::Shape_Program shape{prog};
                    if (shape.recognize(den.second->front())) {
                        print_shape(shape);
                        view_server.display_shape(shape);
                        is_shape = true;
                    }
                }
                if (!is_shape) {
                    for (auto e : *den.second)
                        std::cout << e << "\n";
                }
            }
        } catch (std::exception& e) {
            sys->message("ERROR: ", e);
        }
    }
    view_server.exit();
}

void interactive_mode(
    curv::System& sys, const curv::geom::viewer::Viewer_Config& opts)
{
    (void) opts; // TODO
    sys.use_colour_ = true;
    std::thread repl_thread(repl, &sys);
    view_server.run(opts);
    if (repl_thread.joinable())
        repl_thread.join();
}
