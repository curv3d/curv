// Copyright 2014 Patricio Gonzalez Vivo
// Licensed under the 3-Clause BSD Licence:
// https://opensource.org/licenses/BSD-3-Clause
#include <sys/stat.h>
#include <unistd.h>

#include <map>
#include <thread>
#include <mutex>
#include <atomic>
#include <iostream>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_transform_2d.hpp>
#include <glm/gtx/rotate_vector.hpp>

#include "tools/fs.h"
#include "app.h"
#include "tools/text.h"
#include "tools/geom.h"
#include "gl/shader.h"
#include "gl/vbo.h"
#include "gl/texture.h"
#include "gl/pingpong.h"
#include "gl/uniform.h"
#include "3d/camera.h"
#include "types/shapes.h"
#include <libcurv/geom/viewer/viewer.h>

namespace curv { namespace geom { namespace viewer {

// GLOBAL VARIABLES
//============================================================================
//

//  List of FILES to watch and the variable to communicate that between process
struct WatchFile {
    std::string type;
    std::string path;
    bool vFlip;
    int lastChange;
};
std::vector<WatchFile> files;
std::mutex filesMutex;
int fileChanged;

UniformList uniforms;
std::mutex uniformsMutex;

std::string screenshotFile = "";
std::mutex screenshotMutex;

//  CAMERA
Camera cam;
float lat = 180.0;
float lon = 0.0;
glm::mat3 u_view2d = glm::mat3(1.);
// These are the 'view3d' uniforms.
// Note: the up3d vector must be orthogonal to (eye3d - centre3d),
// or rotation doesn't work correctly.
glm::vec3 u_centre3d = glm::vec3(0.,0.,0.);
// The following initial value for 'eye3d' is derived by starting with [0,0,6],
// then rotating 30 degrees around the X and Y axes.
glm::vec3 u_eye3d = glm::vec3(2.598076,3.0,4.5);
// The initial value for up3d is derived by starting with [0,1,0], then
// applying the same rotations as above, so that up3d is orthogonal to eye3d.
glm::vec3 u_up3d = glm::vec3(-0.25,0.866025,-0.433013);

//  ASSETS
Vbo* vbo;
int iGeom = -1;
glm::mat4 model_matrix = glm::mat4(1.);
std::string outputFile = "";

// Textures
std::map<std::string,Texture*> textures;
bool vFlip = true;

// Include folders
std::vector<std::string> include_folders;

// Backbuffer
PingPong buffer;
Vbo* buffer_vbo;
Shader buffer_shader;

//================================================================= Functions

void screenshot(std::string file);

void onExit();
void printUsage(const char *);

// Main program
//============================================================================
int
Viewer::main(Viewer* viewer)
{
    const char* argv[2];
    argv[0] = "curv";
    argv[1] = nullptr;
    int argc = 1;

    u_centre3d = glm::vec3(0.,0.,0.);
    u_eye3d = glm::vec3(2.598076,3.0,4.5);
    u_up3d = glm::vec3(-0.25,0.866025,-0.433013);

    bool headless = false;
    for (int i = 1; i < argc ; i++) {
        std::string argument = std::string(argv[i]);

        if (   std::string(argv[i]) == "--headless" ) {
            headless = true;
        }
    }

    // Initialize openGL context
    viewer->initGL (viewer->window_pos_and_size_, headless);

    struct stat st; // for files to watch
    float timeLimit = -1.0f; //  Time limit
    int textureCounter = 0; // Number of textures to load

    // Adding default deines
    viewer->defines_.push_back("GLSLVIEWER 1");

    //Load the the resources (textures)
    for (int i = 1; i < argc ; i++){
        std::string argument = std::string(argv[i]);

        if (argument == "-l" ||
                 argument == "--headless") {
        }
        else if ( argument == "-v" || 
                  argument == "--verbose" ) {
            viewer->verbose_ = true;
        }
        else if (argument == "-s" || argument == "--sec") {
            i++;
            argument = std::string(argv[i]);
            timeLimit = toFloat(argument);
            std::cout << "// Will exit in " << timeLimit << " seconds." << std::endl;
        }
        else if (argument == "-o") {
            i++;
            argument = std::string(argv[i]);
            if (haveExt(argument, "png")) {
                outputFile = argument;
                std::cout << "// Will save screenshot to " << outputFile  << " on exit." << std::endl;
            }
            else {
                std::cerr << "At the moment screenshots only support PNG formats" << std::endl;
            }
        }
        else if (argument == "-vFlip") {
            vFlip = false;
        }
        else if (   haveExt(argument,"png") || haveExt(argument,"PNG") ||
                    haveExt(argument,"jpg") || haveExt(argument,"JPG") ||
                    haveExt(argument,"jpeg") || haveExt(argument,"JPEG")) {
            if (stat(argument.c_str(), &st) != 0) {
                std::cerr << "Error watching file " << argument << std::endl;
            }
            else {
                Texture* tex = new Texture();

                if (tex->load(argument, vFlip)) {
                    std::string name = "u_tex"+toString(textureCounter);
                    textures[name] = tex;

                    WatchFile file;
                    file.type = "image";
                    file.path = argument;
                    file.lastChange = st.st_mtime;
                    file.vFlip = vFlip;
                    files.push_back(file);

                    std::cout << "// Loading " << argument << " as the following uniform: " << std::endl;
                    std::cout << "//    uniform sampler2D " << name  << "; // loaded"<< std::endl;
                    std::cout << "//    uniform vec2 " << name  << "Resolution;"<< std::endl;
                    textureCounter++;
                }
            }
        }
        else if (argument.find("-D") == 0) {
            std::string define = argument.substr(2);
            viewer->defines_.push_back(define);
        }
        else if (argument.find("-I") == 0) {
            std::string include = argument.substr(2);
            include_folders.push_back(include);
        }
        else if (argument.find("-") == 0) {
            std::string parameterPair = argument.substr(argument.find_last_of('-')+1);
            i++;
            argument = std::string(argv[i]);
            if (stat(argument.c_str(), &st) != 0) {
                std::cerr << "Error watching file " << argument << std::endl;
            }
            else {
                Texture* tex = new Texture();
                if (tex->load(argument, vFlip)) {
                    textures[parameterPair] = tex;

                    WatchFile file;
                    file.type = "image";
                    file.path = argument;
                    file.lastChange = st.st_mtime;
                    file.vFlip = vFlip;
                    files.push_back(file);

                    std::cout << "// Loading " << argument << " as the following uniform: " << std::endl;
                    std::cout << "//     uniform sampler2D " << parameterPair  << "; // loaded"<< std::endl;
                    std::cout << "//     uniform vec2 " << parameterPair  << "Resolution;"<< std::endl;
                }
            }
        }
    }

    // Start working on the GL context
    viewer->setup();

    // Render Loop
    while (viewer->draw_frame());

    //glfwGetWindowPos(window,
    //    &viewer->window_pos_and_size_.x, &viewer->window_pos_and_size_.y);
    glfwGetWindowSize(window,
        &viewer->window_pos_and_size_.w, &viewer->window_pos_and_size_.z);
    onExit();
    viewer->on_close();
    return 0;
}

bool Viewer::draw_frame()
{
    if (!isGL())
        return false;

    // Update
    updateGL();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // Something change??
    if (!next_frame())
        return false;

    // Draw
    draw();

    // Swap the buffers
    renderGL();

    return true;
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
    if (iGeom == -1){
        vbo = rect(0.0,0.0,1.0,1.0).getVbo();
    }
    else {
        Mesh model;
        model.load(files[iGeom].path);
        vbo = model.getVbo();
        glm::vec3 toCentroid = getCentroid(model.getVertices());
        // model_matrix = glm::scale(glm::vec3(0.001));
        model_matrix = glm::translate(-toCentroid);
    }

    //  Build shader;
    //
    vertSource_ = vbo->getVertexLayout()->getDefaultVertShader();
    shader_.load(fragsrc_, vertSource_, defines_, verbose_);

    cam.setViewport(getWindowWidth(), getWindowHeight());
    cam.setPosition(glm::vec3(0.0,0.0,-3.));

    buffer.allocate(getWindowWidth(), getWindowHeight(), false);

    buffer_vbo = rect(0.0,0.0,1.0,1.0).getVbo();
    std::string buffer_vert = "#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
attribute vec4 a_position;\n\
\n\
void main(void) {\n\
    gl_Position = a_position;\n\
}";

std::string buffer_frag = "#ifdef GL_ES\n\
precision mediump float;\n\
#endif\n\
\n\
uniform sampler2D u_buffer;\n\
uniform vec2 u_resolution;\n\
\n\
void main() {\n\
    vec2 st = gl_FragCoord.xy/u_resolution.xy;\n\
    gl_FragColor = texture2D(u_buffer, st);\n\
}";
    buffer_shader.load(buffer_frag, buffer_vert, defines_);

    // Turn on Alpha blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);

    // Clear the background
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

}

void Viewer::draw()
{
    if (shader_.needBackbuffer()) {
        buffer.swap();
        buffer.src->bind();
    }

    shader_.use();

    // Pass uniforms
    shader_.setUniform("u_resolution", getWindowWidth(), getWindowHeight());
    if (shader_.needTime()) {
        shader_.setUniform("u_time", float(getTime()));
    }
    if (shader_.needDelta()) {
        shader_.setUniform("u_delta", float(getDelta()));
    }
    if (shader_.needDate()) {
        shader_.setUniform("u_date", getDate());
    }
    if (shader_.needMouse()) {
        shader_.setUniform("u_mouse", getMouseX(), getMouseY());
    }
    if (shader_.need_iMouse()) {
        shader_.setUniform("iMouse", get_iMouse());
    }
    if (shader_.needView2d()) {
        shader_.setUniform("u_view2d", u_view2d);
    }
    if (shader_.needView3d()) {
        shader_.setUniform("u_eye3d", u_eye3d);
        shader_.setUniform("u_centre3d", u_centre3d);
        shader_.setUniform("u_up3d", u_up3d);
    }

    for (UniformList::iterator it=uniforms.begin(); it!=uniforms.end(); ++it) {
        if (it->second.bInt) {
            shader_.setUniform(it->first, int(it->second.value[0]));
        }
        else {
            shader_.setUniform(it->first, it->second.value, it->second.size);
        }
    }

    glm::mat4 mvp = glm::mat4(1.);
    if (iGeom != -1) {
        shader_.setUniform("u_eye", -cam.getPosition());
        shader_.setUniform("u_normalMatrix", cam.getNormalMatrix());

        shader_.setUniform("u_modelMatrix", model_matrix);
        shader_.setUniform("u_viewMatrix", cam.getViewMatrix());
        shader_.setUniform("u_projectionMatrix", cam.getProjectionMatrix());

        mvp = cam.getProjectionViewMatrix() * model_matrix;
    }
    shader_.setUniform("u_modelViewProjectionMatrix", mvp);

    // Pass Textures Uniforms
    unsigned int index = 0;
    for (std::map<std::string,Texture*>::iterator it = textures.begin(); it!=textures.end(); ++it) {
        shader_.setUniform(it->first, it->second, index);
        shader_.setUniform(it->first+"Resolution", it->second->getWidth(), it->second->getHeight());
        index++;
    }

    if (shader_.needBackbuffer()) {
        shader_.setUniform("u_backbuffer", buffer.dst, index);
    }

    vbo->draw(&shader_);

    if (shader_.needBackbuffer()) {
        buffer.src->unbind();
        buffer_shader.use();
        buffer_shader.setUniform("u_resolution",getWindowWidth(), getWindowHeight());
        buffer_shader.setUniform("u_modelViewProjectionMatrix", mvp);
        buffer_shader.setUniform("u_buffer", buffer.src, index++);
        buffer_vbo->draw(&buffer_shader);
    }

    if (screenshotFile != "") {
        screenshot(screenshotFile);
        screenshotFile = "";
    }
}

// Rendering Thread
//============================================================================

void onKeyPress(int _key)
{
    if (_key == 's' || _key == 'S') {
        screenshot(outputFile);
    }

    if (_key == 'q' || _key == 'Q') {
        glfwSetWindowShouldClose(window, GL_TRUE);
    }
}

void onMouseMove(float _x, float _y)
{

}

void onMouseClick(float _x, float _y, int _button)
{

}

void onScroll(float _yoffset)
{
    // Vertical scroll button zooms u_view2d and view3d.
    /* zoomfactor 2^(1/4): 4 scroll wheel clicks to double in size. */
    constexpr float zoomfactor = 1.1892;
    if (_yoffset != 0) {
        float z = pow(zoomfactor, _yoffset);

        // zoom view2d
        glm::vec2 zoom = glm::vec2(z,z);
        glm::vec2 origin = {getWindowWidth()/2, getWindowHeight()/2};
        u_view2d = glm::translate(u_view2d, origin);
        u_view2d = glm::scale(u_view2d, zoom);
        u_view2d = glm::translate(u_view2d, -origin);

        // zoom view3d
        u_eye3d = u_centre3d + (u_eye3d - u_centre3d)*z;
    }
}

void onMouseDrag(float _x, float _y, int _button)
{
    if (_button == 1){
        // Left-button drag is used to rotate geometry.
        float dist = glm::length(cam.getPosition());
        lat -= getMouseVelX();
        lon -= getMouseVelY()*0.5;
        cam.orbit(lat,lon,dist);
        cam.lookAt(glm::vec3(0.0));

        // Left-button drag is used to pan u_view2d.
        u_view2d = glm::translate(u_view2d, -getMouseVelocity());

        // Left-button drag is used to rotate eye3d around centre3d.
        // One complete drag across the screen width equals 360 degrees.
        constexpr double tau = 6.283185307179586;
        u_eye3d -= u_centre3d;
        u_up3d -= u_centre3d;
        // Rotate about vertical axis, defined by the 'up' vector.
        float xangle = (getMouseVelX() / getWindowWidth()) * tau;
        u_eye3d = glm::rotate(u_eye3d, -xangle, u_up3d);
        // Rotate about horizontal axis, which is perpendicular to
        // the (centre3d,eye3d,up3d) plane.
        float yangle = (getMouseVelY() / getWindowHeight()) * tau;
        glm::vec3 haxis = glm::cross(u_eye3d-u_centre3d, u_up3d);
        u_eye3d = glm::rotate(u_eye3d, -yangle, haxis);
        u_up3d = glm::rotate(u_up3d, -yangle, haxis);
        //
        u_eye3d += u_centre3d;
        u_up3d += u_centre3d;
    } else {
        // Right-button drag is used to zoom geometry.
        float dist = glm::length(cam.getPosition());
        dist += (-.008f * getMouseVelY());
        if(dist > 0.0f){
            cam.setPosition( -dist * cam.getZAxis() );
            cam.lookAt(glm::vec3(0.0));
        }

        // TODO: rotate view2d.

        // pan view3d.
        float dist3d = glm::length(u_eye3d - u_centre3d);
        glm::vec3 voff = glm::normalize(u_up3d)
            * (getMouseVelY()/getWindowHeight()) * dist3d;
        u_centre3d -= voff;
        u_eye3d -= voff;
        glm::vec3 haxis = glm::cross(u_eye3d-u_centre3d, u_up3d);
        glm::vec3 hoff = glm::normalize(haxis)
            * (getMouseVelX()/getWindowWidth()) * dist3d;
        u_centre3d += hoff;
        u_eye3d += hoff;
    }
}

void onViewportResize(int _newWidth, int _newHeight)
{
    cam.setViewport(_newWidth,_newHeight);
    buffer.allocate(_newWidth,_newHeight);
}

void screenshot(std::string _file)
{
    if (_file != "" && isGL()) {
        unsigned char* pixels = new unsigned char[getWindowWidth()*getWindowHeight()*4];
        glReadPixels(0, 0, getWindowWidth(), getWindowHeight(), GL_RGBA, GL_UNSIGNED_BYTE, pixels);
        Texture::savePixels(_file, pixels, getWindowWidth(), getWindowHeight());
        std::cout << "// Screenshot saved to " << _file << std::endl;
    }
}

void onExit()
{
    // Take a screenshot if it need
    screenshot(outputFile);

    // clear screen
    glClear( GL_COLOR_BUFFER_BIT );

    // close openGL instance
    closeGL();

    // DELETE RESOURCES
    for (std::map<std::string,Texture*>::iterator i = textures.begin(); i != textures.end(); ++i) {
        delete i->second;
        i->second = NULL;
    }
    textures.clear();
    delete vbo;
}

void printUsage(const char * executableName)
{
    std::cerr << "Usage: " << executableName << " <shader>.frag [<shader>.vert] [<mesh>.(obj/.ply)] [<texture>.(png/jpg)] [-<uniformName> <texture>.(png/jpg)] [-vFlip] [-x <x>] [-y <y>] [-w <width>] [-h <height>] [-l] [--square] [-s/--sec <seconds>] [-o <screenshot_file>.png] [--headless] [-I<include_folder>] [-D<define>] [-v/--verbose] [--help]\n";
}

}}}
