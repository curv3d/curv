// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_COLOUR_H
#define LIBCURV_COLOUR_H

#include <glm/vec3.hpp>

namespace curv {

glm::vec3 linearRGB_to_sRGB(glm::vec3);
glm::vec3 sRGB_to_linearRGB(glm::vec3);

} // namespace curv
#endif // header guard
