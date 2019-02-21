// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_IMPORT_H
#define LIBCURV_GEOM_IMPORT_H

namespace curv {

struct System;

namespace geom {

// Add importers for graphical file formats.
void add_importers(System&);

}} // namespace
#endif // include guard
