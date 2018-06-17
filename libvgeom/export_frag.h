// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBVGEOM_EXPORT_FRAG_H
#define LIBVGEOM_EXPORT_FRAG_H

#include <ostream>

namespace vgeom {

struct Shape_Recognizer;

void export_frag(const Shape_Recognizer&, std::ostream&);

} // namespace
#endif // header guard
