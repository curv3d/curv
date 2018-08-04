// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_OUTPUT_FILE_H
#define LIBCURV_OUTPUT_FILE_H

#include <memory>
#include <ostream>
#include <libcurv/filesystem.h>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/file_descriptor.hpp>

namespace curv {

// Output_File is used to mediate between a client, who is calling a
// file export function (the 'exporter'), and the exporter.
// The client can provide either a file name, or an open std::ostream.
// The exporter can request either a file name, or an open std::ostream.
// These choices can be different. The data is not written to the sink
// specified by the client until commit() is called. If an exception is
// thrown before this happens, we will not leave behind a partially written
// file, or write partial data to the client's output stream. A temp file is
// used to buffer the data until commit() is called.
struct Output_File
{
    using tstream =
        boost::iostreams::stream<boost::iostreams::file_descriptor_sink>;

    // Client state.
    Filesystem::path path_{};
    std::ostream* ostream_ = nullptr;

    // Following are used by client, who is calling a file export function.
    // Client specifies if she wants the data to be written to a named file,
    // or to an already open output stream.
    Output_File(Filesystem::path p) : path_(std::move(p)) {}
    Output_File(std::ostream* s) : ostream_(s) {}
    Output_File() {}
    void set_path(Filesystem::path p) { std::swap(path_, p); }
    void set_ostream(std::ostream* s) { ostream_ = s; }

    // Exporter state.
    Filesystem::path tempfile_path_{};
    tstream tempfile_ostream_{};

    // Following are used by the export function ("exporter").
    // Exporter specifies if he wants a filename, or an open output stream.
    // Either use path() to get a filename, or open() followed by ostream()
    // to write to an output stream.
    void open();
    std::ostream& ostream() { return tempfile_ostream_; }
    const Filesystem::path& path();

    // After the exporter has written all of the data, the client calls
    // commit(), which atomically updates the output file, or writes the
    // data to the output stream specified by the client. If commit() is
    // not called before the Output_File is destroyed, none of this happens.
    void commit();

    ~Output_File();
};

} // namespace
#endif // header guard
