// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef VIEW_SERVER_H
#define VIEW_SERVER_H

#include <mutex>
#include <condition_variable>

#include <libcurv/die.h>
#include <libcurv/viewed_shape.h>
#include <libcurv/geom/viewer/viewer.h>

#include "shapes.h"

// View_Server implements a client/server architecture for running a Curv
// viewer window. The 'client' and 'server' run in two different threads within
// the same process, and share access to a View_Server object.
//
// This implements the semantics required by the REPL (curv interactive mode)
// and by live-editing mode. The server keeps running until the client calls
// exit(). Closing the viewer window doesn't terminate the server:
// the window is reopened the next time the client calls display_shape().
struct View_Server
{
private:
    // Internally, I use send(), receive(), reply() to communicate between
    // the client and the server.
    enum class Request {
        k_none,
        k_display_shape,
        k_exit
    };
    Request request_;
    std::mutex request_mutex_;
    std::condition_variable request_condition_;
    curv::Viewed_Shape request_shape_;
    struct Message {
        View_Server& server_;
        std::unique_lock<std::mutex> lock_;
        Request id_;
        Message(View_Server& s)
        :
            server_(s),
            lock_(s.request_mutex_)
        {}
        void receive()
        {
            server_.request_condition_.wait(lock_,
                [&]{return server_.request_ != Request::k_none;});
            id_ = server_.request_;
        }
        bool try_receive()
        {
            using namespace std::chrono_literals;
            bool b = server_.request_condition_.wait_for(lock_, 0ms,
                [&]{return server_.request_ != Request::k_none;});
            if (b)
                id_ = server_.request_;
            return b;
        }
        void reply()
        {
            server_.request_ = Request::k_none; // indicate we are ready for another request

            // Manual unlocking is done before notifying, to avoid waking up
            // the waiting thread only to block again (see notify_one for details)
            lock_.unlock();
            server_.request_condition_.notify_one();
        }
        ~Message()
        {
            // What should the destructor do if we received a message
            // but have not replied?
        }
    };
    curv::geom::viewer::Viewer view_;
    bool lazy_;
    void send(Request r)
    {
        {
            std::lock_guard<std::mutex> lock(request_mutex_);
            request_ = r;
        }
        request_condition_.notify_one();
        // wait for the response
        {
            using namespace std::chrono_literals;
            std::unique_lock<std::mutex> lock(request_mutex_);
            if (lazy_ && view_.is_open()) {
                // In lazy mode, we post empty glfw events to wake main thread,
                // as it might be sleeping waiting for glfw events. Keep posting
                // until it acknowledges the message.
                do glfwPostEmptyEvent();
                while (!request_condition_.wait_for(lock, 10ms,
                            [&]{return request_ == Request::k_none;}));
            }
            else {
                request_condition_.wait(lock,
                        [&]{return request_ == Request::k_none;});
            }
        }
    }
public:
    View_Server()
    {
    }
    // Called by client thread. If the viewer window is not open, then open it,
    // and display the shape. Otherwise, change the shape displayed in the
    // current viewer window.
    void display_shape(const curv::Shape_Program& shape)
    {
        request_shape_ = curv::Viewed_Shape(shape, view_.config_);
        send(Request::k_display_shape);
        //assert(request_shape_.empty());
    }
    // Called by client thread. Close the viewer window, if open, and cause
    // the server to stop running (the run() function will return).
    void exit()
    {
        send(Request::k_exit);
    }
    // Called by server thread. Run the server, which is responsible for opening
    // the viewer window (when display_shape() is called by the client) and
    // calling OpenGL to render each frame. On macOS, this must be run in the
    // main thread. The server keeps running until the client calls exit().
    // If the user closes the window, the server keeps running, and the next
    // call to display_shape() reopens the window.
    void run(const curv::geom::viewer::Viewer_Config& config)
    {
        view_.config_ = config;
        lazy_ = config.lazy_;
        bool exiting = false;
        for (;;) {
            Message msg(*this);
            msg.receive();
            if (msg.id_ == Request::k_exit) {
                msg.reply();
                break;
            }
            if (msg.id_ == Request::k_display_shape) {
                assert(!request_shape_.empty());
                view_.set_shape(std::move(request_shape_));
                msg.reply();
                view_.open();
                do {
                    // Inner loop while viewer window is open.
                    Message imsg(*this);
                    if (imsg.try_receive()) {
                        if (imsg.id_ == Request::k_exit) {
                            exiting = true;
                            imsg.reply();
                            break;
                        }
                        else if (imsg.id_ == Request::k_display_shape) {
                            assert(!request_shape_.empty());
                            view_.set_shape(std::move(request_shape_));
                            imsg.reply();
                        }
                        else
                            curv::die("bad message");
                    }
                } while (view_.draw_frame());
                view_.close();
                if (exiting)
                    break;
            }
        }
    }
};

#endif // header guard
