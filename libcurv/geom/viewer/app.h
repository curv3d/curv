// Copyright 2014 Patricio Gonzalez Vivo
// Licensed under the 3-Clause BSD Licence:
// https://opensource.org/licenses/BSD-3-Clause
#pragma once

#include <gl/opengl.h>
#include <glm/glm.hpp>

namespace curv { namespace geom { namespace viewer {

glm::ivec2 getScreenSize();
float getPixelDensity();

glm::ivec4 getViewport();
int getWindowWidth();
int getWindowHeight();

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

}}}
