// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

// Based on glslViewer, which is:
// Copyright 2014 Patricio Gonzalez Vivo
// Licensed under the 3-Clause BSD Licence:
// https://opensource.org/licenses/BSD-3-Clause

#ifndef LIBCURV_GEOM_VIEWER_VIEWER_H
#define LIBCURV_GEOM_VIEWER_VIEWER_H

#include <libcurv/frag.h>
#include <libcurv/viewed_shape.h>
#include "shader.h"
#include "vbo.h"
#include <glm/glm.hpp>

namespace curv {
struct Shape_Program;

namespace geom {

namespace viewer {

struct Viewer_Config : public Frag_Export
{
    bool verbose_ = false;
    bool lazy_ = false;
};

struct Viewer
{
    /*--- PUBLIC API ---*/
    Viewer();
    Viewer(const Viewer_Config&);

    // Set the current shape. May be called at any time, before opening the
    // window, or while the window is open.
    void set_shape(const Shape_Program&);
    void set_shape(const Shape_Program&, const Frag_Export&);
    void set_shape(Viewed_Shape);

    bool is_open() { return window_ != nullptr; }

    // Reset camera to initial position.
	enum viewtype {home,upside, downside, leftside, rightside, frontside, backside};
    void reset_view(viewtype view);

    // Open window: initialize OpenGL, create the window. Idempotent.
    void open();

    // Called repeatedly in a loop once the window is open.
    // Returns false when it is time to exit the loop (user has closed window).
    bool draw_frame();

    // Close window. Idempotent.
    void close();

    ~Viewer();

    // Open a Viewer window on the current shape, and run until the window
    // is closed by the user. It is equivalent to:
    //     open();
    //     while (draw_frame());
    //     close();
    void run();

    /*--- PARAMETER STATE, can set before window is opened ---*/
    Viewer_Config config_;
    glm::ivec4 window_pos_and_size_{0.,0.,500.,500.};
    bool headless_{false};

    /*--- INTERNAL STATE ---*/

    Viewed_Shape shape_{};
    Shader shader_{};
    std::string vertSource_{};
    GLFWwindow* window_ = nullptr;
    glm::ivec4 viewport_;
    float fPixelDensity_ = 1.0;
    Vbo* vbo_ = nullptr;
    double current_time_ = 0.0;
    struct {
        double last_reported_ = 0.0;
        double delta_time_ = 0.0;
        unsigned frames_ = 0;
        void reset() {
            last_reported_ = glfwGetTime();
            delta_time_ = 0.0;
            frames_ = 0;
        }
    } fps_;
    GLuint vao_; // a Vertex Array Object
    bool hud_ = false;
    bool error_ = false;
    unsigned num_errors_ = 0;

    // The 2D camera position.
    glm::mat3 u_view2d_ {};

    // The 3D camera position. See reset_view() for more information.
    glm::vec3 u_centre3d_ {};
    glm::vec3 u_eye3d_ {};
    glm::vec3 u_up3d_ {};

    typedef struct {
        float     x,y;
        int       button;
        glm::vec2 velocity;
        glm::vec2 drag;
    } Mouse;
    Mouse mouse_;

    // INTERNAL FUNCTIONS
    void initGL(glm::ivec4 &_viewport, bool _headless = false);
    void setup();
    void onKeyPress(int, int);
    void onMouseMove(double, double);
    void onScroll(float);
    void onMouseDrag(float, float, int);
    void render();
    void swap_buffers();
    void poll_events();
    void measure_time();
    void closeGL();
    float getPixelDensity();
    void onExit();
    void setWindowSize(int, int);
    int getWindowWidth();
    int getWindowHeight();
};

}}} // namespace
#endif // header guard
