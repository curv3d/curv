// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_IO_IMPORT_H
#define LIBCURV_IO_IMPORT_H

namespace curv {

struct System;

namespace io {

// Add importers for graphical file formats.
void add_importers(System&);

}} // namespace
#endif // include guard
