// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_IMPORT_H
#define LIBCURV_IMPORT_H

#include <libcurv/filesystem.h>
#include <libcurv/value.h>

namespace curv {

struct Context;
struct Program;

// Import a source file of any type, in the case that the user has explicitly
// specified a pathname. Directories are imported using directory syntax,
// regardless of the filename. Otherwise, if the filename has a recognized
// extension, then the import function for that extension is used.
// Otherwise, we default to Curv syntax.
void import(const Filesystem::path&, Program&, const Context&);

typedef void (*Importer)(const Filesystem::path&, Program&, const Context&);
Value import_value(Importer, const Filesystem::path&, const Context&);

// Import a Curv language source file.
void curv_import(const Filesystem::path& path, Program&, const Context& cx);

// Import a directory as a record value, using "directory syntax".
void dir_import(const Filesystem::path&, Program&, const Context&);

}
#endif
