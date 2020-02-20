// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_FRAG_H
#define LIBCURV_FRAG_H

#include <libcurv/render.h>
#include <ostream>

namespace curv {

struct Shape_Program;

void export_frag(const Shape_Program&, const Render_Opts&, std::ostream&);

} // namespace
#endif // header guard
