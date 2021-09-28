// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/io/import.h>
#include <libcurv/system.h>
#include <libcurv/exception.h>

namespace curv { namespace io {

void import_png(const Filesystem::path &path, Program&, const Context& cx)
{
    (void) path;
    throw Exception(cx, ".PNG file import is not implemented");
}

void add_importers(System& sys)
{
    sys.importers_[".png"] = import_png;
}

}} // namespaces
