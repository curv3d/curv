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

#ifndef VIEW_SERVER_H
#define VIEW_SERVER_H

// extern "C" {
// #include <string.h>
// #include <signal.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <fcntl.h>
// #include <errno.h>
// #include <unistd.h>
// #include <sys/types.h>
// #include <sys/stat.h>
// #include <sys/wait.h>
// }
//#include <iostream>
//#include <fstream>
#include <condition_variable>
#include <functional>
// #include <replxx.hxx>

// #include "export.h"
// #include "progdir.h"
// #include "cscript.h"
#include "shapes.h"
// #include <libcurv/geom/tempfile.h>
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

// encapsulates the viewer thread
struct View_Server
{
    enum class Request {
        k_none,
        k_display_shape,
        k_exit
    };
    Request request;
    std::mutex request_mutex;
    std::condition_variable request_condition;
    std::string request_shape;
    bool exiting_{false};
    struct Message {
        View_Server& server_;
        std::unique_lock<std::mutex> lock_;
        Request id_;
        Message(View_Server& s)
        :
            server_(s),
            lock_(s.request_mutex)
        {}
        void receive()
        {
            server_.request_condition.wait(lock_,
                [&]{return server_.request != Request::k_none;});
            id_ = server_.request;
        }
        bool try_receive()
        {
            using namespace std::chrono_literals;
            bool b = server_.request_condition.wait_for(lock_, 0ms,
                [&]{return server_.request != Request::k_none;});
            if (b)
                id_ = server_.request;
            return b;
        }
        void reply()
        {
            server_.request = Request::k_none; // indicate we are ready for another request

            // Manual unlocking is done before notifying, to avoid waking up
            // the waiting thread only to block again (see notify_one for details)
            lock_.unlock();
            server_.request_condition.notify_one();
        }
        ~Message()
        {
            // What should the destructor do if we received a message
            // but have not replied?
        }
    };
    struct View : public curv::geom::viewer::Viewer
    {
        View_Server& server_;
        View(View_Server& s)
        :
            server_(s)
        {}
        virtual bool next_frame() override
        {
            Message msg(server_);
            if (msg.try_receive()) {
                if (msg.id_ == Request::k_exit) {
                    server_.exiting_ = true;
                    msg.reply();
                    return false;
                }
                if (msg.id_ == Request::k_display_shape) {
                    assert(!server_.request_shape.empty());
                    set_frag(server_.request_shape);
                    msg.reply();
                    return true;
                }
                curv::die("bad message");
            }
            return true;
        }
    } view;
    View_Server()
    :
        view(*this)
    {
    }
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
            request_condition.wait(lock,
                [&]{return request == Request::k_none;});
        }
    }
    void display_shape(curv::geom::Shape_Recognizer& shape)
    {
        request_shape = shape_to_frag(shape);
        send_request(Request::k_display_shape);
        assert(request_shape.empty());
    }
    void exit()
    {
        send_request(Request::k_exit);
    }
    void run()
    {
        for (;;) {
            Message msg(*this);
            msg.receive();
            if (msg.id_ == Request::k_exit) {
                msg.reply();
                break;
            }
            if (msg.id_ == Request::k_display_shape) {
                assert(!request_shape.empty());
                std::swap(view.fragsrc_, request_shape);
                msg.reply();
                view.run();
                if (exiting_)
                    break;
            }
        }
    }
};

#endif // header guard
