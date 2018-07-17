// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef VIEW_SERVER_H
#define VIEW_SERVER_H

#include <condition_variable>
#include <functional>

#include "shapes.h"

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

// View_Server implements a client/server architecture for running a Curv
// viewer window. The 'client' and 'server' run in two different threads within
// the same process, and share access to a View_Server object.
struct View_Server
{
private:
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
public:
    View_Server()
    :
        view(*this)
    {
    }
    // Called by client thread. If the viewer window is not open, then open it,
    // and display the shape. Otherwise, change the shape displayed in the
    // current viewer window.
    void display_shape(curv::geom::Shape_Recognizer& shape)
    {
        request_shape = shape_to_frag(shape);
        send_request(Request::k_display_shape);
        assert(request_shape.empty());
    }
    // Called by client thread. Close the viewer window, if open, and cause
    // the server to stop running (the run() function will return).
    void exit()
    {
        send_request(Request::k_exit);
    }
    // Called by server thread. Run the server, which is responsible for
    // opening the viewer window (on request from the client) and making
    // OpenGL calls to render each frame. On macOS, this must be run in the
    // main thread.
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
