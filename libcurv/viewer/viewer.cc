// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

// Based on glslViewer, which is:
// Copyright 2014 Patricio Gonzalez Vivo
// Licensed under the 3-Clause BSD Licence:
// https://opensource.org/licenses/BSD-3-Clause

#include <libcurv/viewer/viewer.h>

#include <iostream>
#include <sstream>
#include <chrono>
#include <thread>

#include <libcurv/context.h>
#include <libcurv/die.h>
#include <libcurv/exception.h>
#include <libcurv/string.h>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "scale_picker.h"
#include "shapes.h"

#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

namespace curv { namespace viewer {

Viewer::Viewer()
{
}

Viewer::Viewer(const Viewer_Config& config)
:
    config_(config)
{
}

void
Viewer::set_shape_no_hud(const Shape_Program& shape, const Render_Opts& opts)
{
    set_shape(Viewed_Shape(shape, opts));
    hud_ = false;
}

void
Viewer::set_shape(Viewed_Shape shape)
{
    // preserve picker state
    if (!shape_.param_.empty()) {
        for (auto pnew = shape.param_.begin();
             pnew != shape.param_.end();
             ++pnew)
        {
            auto pold = shape_.param_.find(pnew->first);
            if (pold != shape_.param_.end()
                && pnew->second.pconfig_.type_ == pold->second.pconfig_.type_)
            {
                pnew.value().pstate_ = pold->second.pstate_;
            }
        }
    }

    shape_ = move(shape);
    hud_ = !shape_.param_.empty();
  #if 0
    // describe sliders on stderr (TODO: remove debug code)
    for (auto& i : shape_.param_) {
        std::cerr << i.first << " :: ";
        i.second.pconfig_.write(std::cerr);
        std::cerr << " = ";
        i.second.pstate_.write(std::cerr, i.second.pconfig_.type_);
        std::cerr << "\n";
    }
  #endif
    if (is_open()) {
        error_ = false;
        num_errors_ = 0;
        shader_.detach(GL_FRAGMENT_SHADER | GL_VERTEX_SHADER);
        if (!shader_.load(shape_.frag_, vertSource_, config_.verbose_))
            error_ = true;
        fps_.reset();
    }
}

void
Viewer::run()
{
    open();
    while (draw_frame());
    close();
}

void Viewer::open()
{
    if (!is_open()) {
        // Set initial default values for centre, eye and up
        reset_view(home);

        // Initialize openGL context
        initGL();

        // Start working on the GL context
        setup();
    }
}

void Viewer::reset_view(viewtype view )
{
    // Reset the 2D camera position.
    u_view2d_ = glm::mat3(1.);

    // Reset the 3D camera position.
    //
    // The 3D camera position assumes that the object being viewed
    // is a centred, axis aligned cube of size 2,
    // extending between -1 and 1 along the X, Y, and Z axes.
    // The rendering code transforms coordinates based on the actual
    // shape's bounding box to get the actual camera position.
    //
    // Note: the up3d vector must be orthogonal to (eye3d - centre3d),
    // or rotation doesn't work correctly.

    // The centre is the origin.
    u_centre3d_ = glm::vec3(0., 0., 0.);
    if (view==upside)
    {
        u_eye3d_ = glm::vec3(0, 6, 0);
        u_up3d_ = glm::vec3(0, 0, -1);
    }
    else if (view==downside)
    {
        u_eye3d_ = glm::vec3(0, -6, 0);
        u_up3d_ = glm::vec3(0, 0, -1);
    }
    else if (view==leftside)
    {
        u_eye3d_ = glm::vec3(-6, 0, 0);
        u_up3d_ = glm::vec3(0, 1, 0);
    }
    else if (view==rightside)
    {
        u_eye3d_ = glm::vec3(6, 0, 0);
        u_up3d_ = glm::vec3(0, 1, 0);
    }
    else if (view==frontside)
    {
        u_eye3d_ = glm::vec3(0, 0, 6);
        u_up3d_ = glm::vec3(0, 1, 0);
    }
    else if (view==backside)
    {
        u_eye3d_ = glm::vec3(0, 0, -6);
        u_up3d_ = glm::vec3(0, 1, 0);
    }
    else
    {
        // 'eye3d' is derived by starting with [0,0,6], then rotating 30 degrees
        // around the X and Y axes.
        u_eye3d_ = glm::vec3(2.598076, 3.0, 4.5);

        // up3d is derived by starting with [0,1,0], then applying
        // the same rotations as above, so that up3d is orthogonal to eye3d.
        u_up3d_ = glm::vec3(-0.25, 0.866025, -0.433013);
    }
}

// Helper to display a little (?) mark which shows a tooltip when hovered.
static void ShowHelpMarker(const char* desc)
{
    ImGui::TextDisabled("(?)");
    if (ImGui::IsItemHovered())
    {
        ImGui::BeginTooltip();
        ImGui::PushTextWrapPos(ImGui::GetFontSize() * 35.0f);
        ImGui::TextUnformatted(desc);
        ImGui::PopTextWrapPos();
        ImGui::EndTooltip();
    }
}

bool Viewer::draw_frame()
{
    if (glfwWindowShouldClose(window_))
        return false;
    poll_events();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
    if (hud_) {
        //ImGui::ShowDemoWindow(&hud_);
        ImGui::PushStyleColor(ImGuiCol_WindowBg, // modify the Light style
            (ImVec4)ImColor(230,230,230,255));
        ImGui::SetNextWindowPos(ImVec2(0,0), ImGuiCond_Once);
        ImGui::SetNextWindowSize(ImVec2(350,0), ImGuiCond_Once);
        ImGui::Begin("Parameters", &hud_, 0);
        for (auto i = shape_.param_.begin();
             i != shape_.param_.end();
             ++i)
        {
            switch (i->second.pconfig_.type_) {
            case Picker::Type::slider:
                ImGui::SliderFloat(i->first.c_str(), &i.value().pstate_.num_,
                    i->second.pconfig_.slider_.low_,
                    i->second.pconfig_.slider_.high_);
                ImGui::SameLine(); ShowHelpMarker(
                    "Click or drag slider to set value.\n"
                    "CTRL+click to edit value as text.\n");
                break;
            case Picker::Type::int_slider:
                ImGui::SliderInt(i->first.c_str(), &i.value().pstate_.int_,
                    i->second.pconfig_.int_slider_.low_,
                    i->second.pconfig_.int_slider_.high_);
                ImGui::SameLine(); ShowHelpMarker(
                    "Click or drag slider to set value.\n"
                    "CTRL+click to edit value as text.\n");
                break;
            case Picker::Type::scale_picker:
                run_scale_picker(i->first.c_str(), &i.value().pstate_.num_);
                ImGui::SameLine(); ShowHelpMarker(
                    "Drag and hold to adjust value.\n"
                    "SHIFT+drag changes value more quickly.\n"
                    "ALT+drag changes value more slowly.\n"
                    "CTRL+click to edit value as text.\n");
                break;
            case Picker::Type::checkbox:
                ImGui::Checkbox(i->first.c_str(), &i.value().pstate_.bool_);
                break;
            case Picker::Type::colour_picker:
                ImGui::ColorEdit3(i->first.c_str(), i.value().pstate_.vec3_,
                    ImGuiColorEditFlags_PickerHueWheel);
                ImGui::SameLine(); ShowHelpMarker(
                    "Click on the coloured square to open a colour picker.\n"
                    "To adjust a colour component, drag on numeric field,\n"
                    "or CTRL+click to edit value as text.\n"
                    "Right-click on the coloured square to set colour space.\n"
                    "Drag coloured square and drop on another coloured square.\n");
                break;
            }
        }
        if (!shape_.param_.empty()) {
            if (ImGui::Button("Reset")) {
                for (auto i = shape_.param_.begin();
                     i != shape_.param_.end();
                     ++i)
                {
                    i.value().pstate_ = i->second.default_state_;
                }
            }
            ImGui::SameLine();
        }
        //ImGui::Checkbox("Power Saver", &config_.lazy_); // TODO: fix
        ImGui::End();
        ImGui::PopStyleColor(1);
    }

    render();
    swap_buffers();
    if (!config_.lazy_)
        measure_time();
    return true;
}

void Viewer::close()
{
    if (is_open()) {
        //glfwGetWindowPos(window_, &window_pos_.x, &window_pos_.y);
        glfwGetWindowSize(window_, &window_size_.x, &window_size_.y);
        onExit();
    }
}

Viewer::~Viewer()
{
    close();
}

void Viewer::setup()
{
    // Prepare viewport
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glEnable(GL_DEPTH_TEST);
    glFrontFace(GL_CCW);

    //  Load Geometry
    //
    vbo_ = rect(0.0,0.0,1.0,1.0).getVbo();

    //  Build shader;
    //
    vertSource_ = vbo_->getVertexLayout()->getDefaultVertShader();
    if (!shader_.load(shape_.frag_, vertSource_, config_.verbose_))
        error_ = true;

    // Turn on Alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    fps_.reset();
}

void Viewer::render()
{
    unsigned cnt = num_errors_;

    if (!error_) ImGui::Render();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (!error_) {
        shader_.use();

        // Pass uniforms
        shader_.setUniform("u_resolution", getWindowWidth(), getWindowHeight());
        if (shader_.needTime()) {
            shader_.setUniform("u_time", float(current_time_));
        }
        if (shader_.needView2d()) {
            shader_.setUniform("u_view2d", u_view2d_);
        }
        if (shader_.needView3d()) {
            shader_.setUniform("u_eye3d", u_eye3d_);
            shader_.setUniform("u_centre3d", u_centre3d_);
            shader_.setUniform("u_up3d", u_up3d_);
        }
        glm::mat4 mvp = glm::mat4(1.);
        shader_.setUniform("u_modelViewProjectionMatrix", mvp);

        for (auto& p : shape_.param_) {
            // TODO: precompute uniform id
            auto& name = p.second.identifier_;
            switch (p.second.pconfig_.type_) {
            case Picker::Type::slider:
            case Picker::Type::scale_picker:
                shader_.setUniform(name.c_str(), float(p.second.pstate_.num_));
                break;
            case Picker::Type::int_slider:
                shader_.setUniform(name.c_str(), float(p.second.pstate_.int_));
                break;
            case Picker::Type::checkbox:
                shader_.setUniform(name.c_str(), int(p.second.pstate_.bool_));
                break;
            case Picker::Type::colour_picker:
                shader_.setUniform(name.c_str(), p.second.pstate_.vec3_, 3);
                break;
            default:
                die("picker with bad sctype");
            }
        }

        vbo_->draw(&shader_);

        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    if (num_errors_ > cnt) error_ = true;
}

void Viewer::onKeyPress(int key, int mods)
{
    bool ctrl = (mods & (GLFW_MOD_CONTROL|GLFW_MOD_SUPER)) != 0;
    if (key == GLFW_KEY_Q || (key == GLFW_KEY_W && ctrl)) {
        glfwSetWindowShouldClose(window_, GL_TRUE);
    }
    else if (key == GLFW_KEY_HOME) { // reset to default
        reset_view(home);
    }
    else if (key == GLFW_KEY_U) {
        reset_view(upside);
    }
    else if (key == GLFW_KEY_D) {
        reset_view(downside);
    }
    else if (key == GLFW_KEY_L) {
        reset_view(leftside);
    }
    else if (key == GLFW_KEY_R) {
        reset_view(rightside);
    }
    else if (key == GLFW_KEY_F) {
        reset_view(frontside);
    }
    else if (key == GLFW_KEY_B) {
        reset_view(backside);
    }
}

void Viewer::onScroll(float _yoffset)
{
    // Vertical scroll button zooms u_view2d_ and view3d.
    /* zoomfactor 2^(1/4): 4 scroll wheel clicks to double in size. */
    constexpr float zoomfactor = 1.1892;
    if (_yoffset != 0) {
        float z = pow(zoomfactor, _yoffset);

        // zoom view2d
        glm::vec2 zoom = glm::vec2(z,z);
        glm::vec2 origin = {getWindowWidth()/2, getWindowHeight()/2};
        u_view2d_ = glm::translate(u_view2d_, origin);
        u_view2d_ = glm::scale(u_view2d_, zoom);
        u_view2d_ = glm::translate(u_view2d_, -origin);

        // zoom view3d
        u_eye3d_ = u_centre3d_ + (u_eye3d_ - u_centre3d_)*z;
    }
}

void Viewer::onMouseDrag(float _x, float _y, int _button)
{
    if (_button == 1){
        // Left-button drag is used to pan u_view2d_.
        u_view2d_ = glm::translate(u_view2d_, -mouse_.velocity);

        // Left-button drag is used to rotate eye3d around centre3d.
        // One complete drag across the screen width equals 360 degrees.
        constexpr double tau = 6.283185307179586;
        u_eye3d_ -= u_centre3d_;
        u_up3d_ -= u_centre3d_;
        // Rotate about vertical axis, defined by the 'up' vector.
        float xangle = (mouse_.velocity.x / getWindowWidth()) * tau;
        u_eye3d_ = glm::rotate(u_eye3d_, -xangle, u_up3d_);
        // Rotate about horizontal axis, which is perpendicular to
        // the (centre3d,eye3d,up3d) plane.
        float yangle = (mouse_.velocity.y / getWindowHeight()) * tau;
        glm::vec3 haxis = glm::cross(u_eye3d_-u_centre3d_, u_up3d_);
        u_eye3d_ = glm::rotate(u_eye3d_, -yangle, haxis);
        u_up3d_ = glm::rotate(u_up3d_, -yangle, haxis);
        //
        u_eye3d_ += u_centre3d_;
        u_up3d_ += u_centre3d_;
    } else {
        // TODO: rotate view2d.

        // pan view3d.
        float dist3d = glm::length(u_eye3d_ - u_centre3d_);
        glm::vec3 voff = glm::normalize(u_up3d_)
            * (mouse_.velocity.y/getWindowHeight()) * dist3d;
        u_centre3d_ -= voff;
        u_eye3d_ -= voff;
        glm::vec3 haxis = glm::cross(u_eye3d_-u_centre3d_, u_up3d_);
        glm::vec3 hoff = glm::normalize(haxis)
            * (mouse_.velocity.x/getWindowWidth()) * dist3d;
        u_centre3d_ += hoff;
        u_eye3d_ += hoff;
    }
}

void Viewer::onExit()
{
    // clear screen
    glClear( GL_COLOR_BUFFER_BIT );

    // close openGL instance
    closeGL();

    // DELETE RESOURCES
    delete vbo_;
}

const char appTitle[] = "curv";

extern "C" void
MessageCallback(
    GLenum source,
    GLenum type,
    GLuint id,
    GLenum severity,
    GLsizei length,
    const GLchar* message,
    const void* userParam)
{
    (void) length;
    Viewer* viewer = (Viewer*)userParam;
    if (viewer->config_.verbose_ || severity == GL_DEBUG_SEVERITY_HIGH) {
        std::cerr << "GL ";
        switch (type) {
        case GL_DEBUG_TYPE_ERROR:
            std::cerr << "ERROR"; break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR:
            std::cerr << "DEPRECATED"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:
            std::cerr << "UNDEFINED"; break;
        case GL_DEBUG_TYPE_PORTABILITY:
            std::cerr << "PORTABILITY"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:
            std::cerr << "PERFORMANCE"; break;
        case GL_DEBUG_TYPE_MARKER:
            std::cerr << "MARKER"; break;
        case GL_DEBUG_TYPE_PUSH_GROUP:
            std::cerr << "PUSH_GROUP"; break;
        case GL_DEBUG_TYPE_POP_GROUP:
            std::cerr << "POP_GROUP"; break;
        case GL_DEBUG_TYPE_OTHER:
            std::cerr << "OTHER"; break;
        default:
            std::cerr << "<"<<type<<">";
        }
        std::cerr << ": src=" << source
            << ",id=" << id
            << ",sev=" << severity
            << " " << message << "\n";
    }
    if (severity == GL_DEBUG_SEVERITY_HIGH) ++viewer->num_errors_;
}

void Viewer::initGL()
{
    glfwSetErrorCallback([](int err, const char* msg)->void {
        std::cerr << "GLFW error 0x"<<std::hex<<err<<std::dec<<": "<<msg<<"\n";
    });
    if(!glfwInit()) {
        std::cerr << "ABORT: GLFW init failed" << std::endl;
        exit(-1);
    }

    // Set window and context parameters.
    geom::glfw_set_context_parameters();
    if (headless_) {
        glfwWindowHint(GLFW_VISIBLE, GL_FALSE);
    }

    // Create window, create OpenGL context, load OpenGL library.
    window_ = glfwCreateWindow(window_size_.x, window_size_.y, appTitle,
        NULL, NULL);
    if(!window_) {
        glfwTerminate();
        std::cerr << "ABORT: GLFW create window failed" << std::endl;
        exit(-1);
    }
    glfwMakeContextCurrent(window_);
    if (!geom::opengl_init()) {
        std::cerr << "ABORT: Can't load OpenGL library\n";
        exit(-1);
    }

    // Enable OpenGL debugging, so I can print messages when errors occur.
    error_ = false;
    num_errors_ = 0;
    if (GLAD_GL_KHR_debug) {
        glEnable(GL_DEBUG_OUTPUT);
        glDebugMessageCallback(MessageCallback, (void*)this);
    }

    // The GL context is now set up and ready for use.

    glfwSwapInterval(1);

    // Create and bind a VAO (Vertex Array Object) for later use.
    glGenVertexArrays(1, &vao_);
    glBindVertexArray(vao_);

    glfwSetWindowUserPointer(window_, (void*)this);

    setWindowSize(window_size_.x, window_size_.y);
    if (!have_window_pos_) {
        glfwGetWindowPos(window_, &window_pos_.x, &window_pos_.y);
        have_window_pos_ = true;
    } else {
        glfwSetWindowPos(window_, window_pos_.x, window_pos_.y);
    }

    // Initialize ImGUI
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.IniFilename = nullptr;
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;  // Enable Keyboard Controls
    //io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;   // Enable Gamepad Controls
    ImGui_ImplGlfw_InitForOpenGL(window_, false);
    ImGui_ImplOpenGL3_Init(geom::glsl_version);
    //ImGui::StyleColorsDark();
    //ImGui::StyleColorsClassic();
    ImGui::StyleColorsLight();

    // Install event callbacks.
    glfwSetWindowSizeCallback(window_,
        [](GLFWwindow* win, int w, int h) {
            Viewer* self = (Viewer*) glfwGetWindowUserPointer(win);
            self->setWindowSize(w,h);
        });

    glfwSetKeyCallback(window_,
        [](GLFWwindow* win, int key, int scancode, int action, int mods) {
            Viewer* self = (Viewer*) glfwGetWindowUserPointer(win);
            ImGuiIO& io = ImGui::GetIO();
            if (key == GLFW_KEY_H && (mods & GLFW_MOD_CONTROL) && action == GLFW_PRESS)
                self->hud_ = !self->hud_;
            else {
                ImGui_ImplGlfw_KeyCallback(win, key, scancode, action, mods);
                if (!io.WantCaptureKeyboard && action == GLFW_PRESS)
                    self->onKeyPress(key, mods);
            }
        });

    glfwSetCharCallback(window_, ImGui_ImplGlfw_CharCallback);

    // callback when a mouse button is pressed or released
    glfwSetMouseButtonCallback(window_,
        [](GLFWwindow* win, int button, int action, int mods) {
            Viewer* self = (Viewer*) glfwGetWindowUserPointer(win);
            ImGuiIO& io = ImGui::GetIO();
            ImGui_ImplGlfw_MouseButtonCallback(win, button, action, mods);
            if (!io.WantCaptureMouse && action == GLFW_PRESS) {
                self->mouse_.drag.x = self->mouse_.x;
                self->mouse_.drag.y = self->mouse_.y;
            }
        });

    glfwSetScrollCallback(window_,
        [](GLFWwindow* win, double xoffset, double yoffset) {
            Viewer* self = (Viewer*) glfwGetWindowUserPointer(win);
            ImGuiIO& io = ImGui::GetIO();
            ImGui_ImplGlfw_ScrollCallback(win, xoffset, yoffset);
            if (!io.WantCaptureMouse)
                self->onScroll(-yoffset * self->fPixelDensity_);
        });

    // callback when the mouse cursor moves
    glfwSetCursorPosCallback(window_,
        [](GLFWwindow* win, double x, double y) {
            Viewer* self = (Viewer*) glfwGetWindowUserPointer(win);
            ImGuiIO& io = ImGui::GetIO();
            if (!io.WantCaptureMouse)
                self->onMouseMove(x, y);
        });

    glfwSetWindowPosCallback(window_,
        [](GLFWwindow* win, int x, int y) {
            Viewer* self = (Viewer*) glfwGetWindowUserPointer(win);
            self->window_pos_.x = x;
            self->window_pos_.y = y;
            if (self->fPixelDensity_ != self->getPixelDensity()) {
                self->setWindowSize(self->viewport_.x, self->viewport_.y);
            }
        });
}

void Viewer::onMouseMove(double x, double y)
{
    // Convert x,y to pixel coordinates relative to viewport.
    // (0,0) is lower left corner.
    y = viewport_.y - y;
    x *= fPixelDensity_;
    y *= fPixelDensity_;
    // mouse_.velocity is the distance the mouse cursor has moved
    // since the last callback, during a drag gesture.
    // mouse_.drag is the previous mouse position, during a drag gesture.
    // Note that mouse_.drag is *not* constrained to the viewport.
    mouse_.velocity.x = x - mouse_.drag.x;
    mouse_.velocity.y = y - mouse_.drag.y;
    mouse_.drag.x = x;
    mouse_.drag.y = y;

    // mouse_.x,mouse_.y is the current cursor position, constrained
    // to the viewport.
    mouse_.x = x;
    mouse_.y = y;
    if (mouse_.x < 0) mouse_.x = 0;
    if (mouse_.y < 0) mouse_.y = 0;
    if (mouse_.x > viewport_.x * fPixelDensity_) mouse_.x = viewport_.x * fPixelDensity_;
    if (mouse_.y > viewport_.y * fPixelDensity_) mouse_.y = viewport_.y * fPixelDensity_;

    /*
     * TODO: the following code would best be moved into the
     * mouse button callback. If you click the mouse button without
     * moving the mouse, then using this code, the mouse click doesn't
     * register until the cursor is moved. (@doug-moen)
     */
    int action1 = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_1);
    int action2 = glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_2);
    int button = 0;

    if (action1 == GLFW_PRESS) button = 1;
    else if (action2 == GLFW_PRESS) button = 2;

    if (mouse_.button == 0 && button != mouse_.button) {
        mouse_.button = button;
    }
    else {
        mouse_.button = button;
    }

    if (mouse_.velocity.x != 0.0 || mouse_.velocity.y != 0.0) {
        if (button != 0) onMouseDrag(mouse_.x,mouse_.y,mouse_.button);
    }
}

void Viewer::measure_time()
{
    double now = glfwGetTime();
    double delta = now - current_time_;
    current_time_ = now;

    fps_.delta_time_ += delta;
    ++fps_.frames_;
    // Don't update the FPS more than once per second.
    if (now - fps_.last_reported_ >= 1.0) {
        fps_.last_reported_ = now;
        double frame_time = fps_.delta_time_ / fps_.frames_;
        fps_.frames_ = 0;
        fps_.delta_time_ = 0.0;

        // display FPS in window title
        char title[sizeof appTitle + 60];
        snprintf(title, sizeof title, "%s ms/frame: %.2f FPS: %.2f",
            appTitle,
            frame_time*1000.0,
            1.0/frame_time);
        glfwSetWindowTitle(window_, title);
    }
}

void Viewer::poll_events()
{
    if (config_.lazy_)
        glfwWaitEvents();
    else
        glfwPollEvents();
}

void Viewer::swap_buffers()
{
    glfwSwapBuffers(window_);
}

void Viewer::closeGL()
{
    //glfwSetWindowShouldClose(window_, GL_TRUE);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glfwDestroyWindow(window_);
    glfwPollEvents(); // work around GLFW bug #1412
    window_ = nullptr;
}

void Viewer::setWindowSize(int _width, int _height)
{
    viewport_.x = _width;
    viewport_.y = _height;
    fPixelDensity_ = getPixelDensity();
    glViewport(0.0, 0.0, (float)getWindowWidth(), (float)getWindowHeight());
}

float Viewer::getPixelDensity()
{
    int window_width, window_height, framebuffer_width, framebuffer_height;
    glfwGetWindowSize(window_, &window_width, &window_height);
    glfwGetFramebufferSize(window_, &framebuffer_width, &framebuffer_height);
    return float(framebuffer_width)/float(window_width);
}

int Viewer::getWindowWidth()
{
    return viewport_.x*fPixelDensity_;
}

int Viewer::getWindowHeight()
{
    return viewport_.y*fPixelDensity_;
}

}} // namespace
