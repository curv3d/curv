// Copyright 2014 Patricio Gonzalez Vivo
// Licensed under the 3-Clause BSD Licence:
// https://opensource.org/licenses/BSD-3-Clause
#pragma once

#include <gl/opengl.h>
#include <glm/glm.hpp>

namespace curv { namespace geom { namespace viewer {

//	SET
//----------------------------------------------
void setWindowSize(int _width, int _height);

//	GET
//----------------------------------------------
glm::ivec2 getScreenSize();
float getPixelDensity();

glm::ivec4 getViewport();
glm::mat4 getOrthoMatrix();
int getWindowWidth();
int getWindowHeight();

glm::vec4 getDate();
double getTime();
double getDelta();
double getFPS();

float getMouseX();
float getMouseY();
glm::vec2 getMousePosition();
float getMouseVelX();
float getMouseVelY();
glm::vec2 getMouseVelocity();
int getMouseButton();
glm::vec4 get_iMouse();

// EVENTS
//----------------------------------------------
void onMouseClick(float _x, float _y, int _button);
void onMouseDrag(float _x, float _y, int _button);
void onViewportResize(int _width, int _height);
void onScroll(float _yoffset);

}}}
