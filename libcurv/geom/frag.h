// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_FRAG_H
#define LIBCURV_GEOM_FRAG_H

#include <ostream>

namespace curv { namespace geom {

struct Shape_Program;

struct Frag_Export
{
    // spatial anti-aliasing via supersampling. aa_==1 means it is turned off.
    int aa_ = 1;
    // temporal anti-aliasing.
    int taa_ = 1;
    // frame duration for animation, needed for TAA.
    double fdur_ = 0.04; // 25 FPS
};

void export_frag(const Shape_Program&, const Frag_Export&, std::ostream&);

}} // namespace
#endif // header guard
