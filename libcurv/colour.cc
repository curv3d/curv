// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/colour.h>
#include <cmath>

namespace curv {

static float lin_to_gamma(float c)
{
  #if 0
    if (c < 0.0031308)
        return (c < 0.0) ? 0.0 : c * 12.92;
    else
        return 1.055 * pow(c, 1.0 / 2.4) - 0.055;
  #else
    return pow(c, 1.0/2.2); // same as Curv mainImage
  #endif
}

glm::vec3 linearRGB_to_sRGB(glm::vec3 lin)
{
    // can this be vectorized?
    return {lin_to_gamma(lin[0]), lin_to_gamma(lin[1]), lin_to_gamma(lin[2])};
}

static inline float gamma_to_lin(float c)
{
  #if 0
    if (c < 0.04045)
        return c / 12.92;
    else
        return pow((c + 0.055)/1.055, 2.4);
  #else
    return pow(c, 2.2); // same as Curv sRGB
  #endif
}

glm::vec3 sRGB_to_linearRGB(glm::vec3 c)
{
    // can this be vectorized?
    return {gamma_to_lin(c[0]), gamma_to_lin(c[1]), gamma_to_lin(c[2])};
}

} // namespace curv
