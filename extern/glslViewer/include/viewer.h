// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef VIEWER_H
#define VIEWER_H

#include <string>
#include <libcurv/filesystem.h>

void viewer_run_frag(std::string shader);
void viewer_spawn_frag(std::string shader);
void viewer_export_png(std::string shader, curv::Filesystem::path png_pathname);

int viewer_main(int argc, const char** argv);

#endif // header guard
