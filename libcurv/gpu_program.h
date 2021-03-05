// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GPU_PROGRAM_H
#define LIBCURV_GPU_PROGRAM_H

#include <libcurv/render.h>
#include <libcurv/shape.h>
#include <libcurv/viewed_shape.h>

namespace curv {

struct GPU_Program
{
    Source_State& sstate_;
    Shared<const Phrase> nub_;
    bool is_2d_;
    bool is_3d_;
    BBox bbox_;
    Viewed_Shape vshape_;

    GPU_Program(Program&);

    // If the value is a shape, fill in most fields and return true.
    // Used with the (Program&) constructor.
    bool recognize(Value, Render_Opts);

    // abstract interface to PROGRAM classes (see At_Program)
    Location location() const;
    System& system() const { return sstate_.system_; }
    Frame* file_frame() const { return sstate_.file_frame_; }
    const Phrase& nub() const { return *nub_; }

    void write_json(std::ostream&) const;
    void write_curv(std::ostream&) const;
};

} // namespace
#endif // header guard
