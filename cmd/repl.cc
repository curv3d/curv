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
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
}
#include <iostream>
#include <fstream>
#include <condition_variable>
#include <replxx.hxx>

#include "export.h"
#include "progdir.h"
#include "cscript.h"
#include "shapes.h"
#include <libcurv/geom/tempfile.h>
#include <libcurv/dtostr.h>
#include <libcurv/analyser.h>
#include <libcurv/context.h>
#include <libcurv/program.h>
#include <libcurv/exception.h>
#include <libcurv/file.h>
#include <libcurv/parser.h>
#include <libcurv/phrase.h>
#include <libcurv/shared.h>
#include <libcurv/system.h>
#include <libcurv/list.h>
#include <libcurv/record.h>
#include <libcurv/version.h>
#include <libcurv/die.h>
#include <libcurv/geom/export_frag.h>
#include <libcurv/geom/shape.h>
#include <libcurv/geom/viewer/viewer.h>

enum class Request {
    k_none,
    k_display_shape,
    k_exit
};
Request request;
std::mutex request_mutex;
std::condition_variable request_condition;
std::string request_shape;

curv::geom::viewer::Viewer view;

void send_request(Request r)
{
    {
        std::lock_guard<std::mutex> lock(request_mutex);
        request = r;
    }
    request_condition.notify_one();
    // wait for the response
    {
        std::unique_lock<std::mutex> lock(request_mutex);
        request_condition.wait(lock, []{return request == Request::k_none;});
    }
}

Request receive_request(bool block)
{
    using namespace std::chrono_literals;
    std::unique_lock<std::mutex> lock(request_mutex);
    if (block)
        request_condition.wait(lock, []{return request != Request::k_none;});
    else if (!request_condition.wait_for(lock, 0ms,
                                         []{return request != Request::k_none;}))
        return Request::k_none;
 
    // after the wait, we hold the lock.
    Request r = request;

    // process request
    if (r == Request::k_display_shape) {
        assert(!request_shape.empty());
        curv::geom::viewer::Viewer view;
        std::swap(view.fragsrc_, request_shape);
        view.run();
    }

    request = Request::k_none; // indicate we are ready for another request
 
    // Manual unlocking is done before notifying, to avoid waking up
    // the waiting thread only to block again (see notify_one for details)
    lock.unlock();
    request_condition.notify_one();
    return r;
}

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
        const char* line = rx.input("curv> ");
        if (line == nullptr) {
            if (errno == EAGAIN) continue;
            std::cout << "\n";
            break;
        }
        if (line[0] != '\0')
            rx.history_add(line);

        auto script = curv::make<CString_Script>("", line);
        try {
            curv::Program prog{*script, *sys};
            prog.compile(&names, nullptr);
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
                    curv::geom::Shape_Recognizer shape(
                        curv::At_Phrase(prog.nub(), nullptr),
                        *sys);
                    if (shape.recognize(den.second->front())) {
                        print_shape(shape);
                        request_shape = shape_to_frag(shape);
                        send_request(Request::k_display_shape);
                        assert(request_shape.empty());
                        is_shape = true;
                    }
                }
                if (!is_shape) {
                    for (auto e : *den.second)
                        std::cout << e << "\n";
                }
            }
        } catch (curv::Exception& e) {
            std::cout << "ERROR: " << e << "\n";
        } catch (std::exception& e) {
            std::cout << "ERROR: " << e.what() << "\n";
        }
    }
    send_request(Request::k_exit);
}

void interactive_mode(curv::System& sys)
{
    std::thread repl_thread(repl, &sys);
    for (;;) {
        Request r = receive_request(true);
        if (r == Request::k_exit)
            break;
    }
    if (repl_thread.joinable())
        repl_thread.join();
    //view.close();
}
