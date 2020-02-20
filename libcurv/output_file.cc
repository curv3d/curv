// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/output_file.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>
extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>
#include <unistd.h>
}
#include <fstream>

namespace curv {

namespace fs = Filesystem;
namespace io = boost::iostreams;

// Create a uniquely-named tempfile in the specified directory.
// The tempfile name ends in `suffix`.
// The file will be created and exclusively opened, preventing race conditions.
// The file name is stored in `path`.
// The open file descriptor is stored in `stream`.
// An exception is thrown on error.
int
maketemp(
    const fs::path& tempdir,
    const std::string& suffix,
    fs::path& path,
    System& sys)
{
    pid_t pid = getpid();
    fs::path trypath;
    for (int i = 0; i < 20; ++i) {
        clock_t now = clock();
        auto name = stringify(",curv-",pid,"-",now,suffix);
        trypath = tempdir / fs::path(name->c_str());
        int fd = open(trypath.c_str(), O_WRONLY|O_CREAT|O_EXCL, 0600);
        if (fd != -1) {
            path = trypath;
            return fd;
        }
        if (errno != EEXIST)
            break;
    }
    throw Exception(At_System(sys), stringify(
        "can't create tempfile ",trypath.c_str(),": ",strerror(errno)));
}

void
Output_File::open()
{
    fs::path tempdir;
    if (path_.empty())
        tempdir = fs::temp_directory_path();
    else
        tempdir = path_.parent_path();
    int fd = maketemp(tempdir, ".tmp", tempfile_path_, system_);
    tempfile_ostream_.open(io::file_descriptor_sink(fd, io::close_handle));
}

const Filesystem::path&
Output_File::path()
{
    open();
    tempfile_ostream_.close();
    return tempfile_path_;
}

void
Output_File::commit()
{
    if (!tempfile_path_.empty()) {
        if (tempfile_ostream_.is_open())
            tempfile_ostream_.close();
        if (path_.empty()) {
            // copy tempfile to ostream_
            std::ifstream tmp(tempfile_path_.c_str());
            *ostream_ << tmp.rdbuf();
        } else {
            if (rename(tempfile_path_.c_str(), path_.c_str()) == -1) {
                throw Exception(At_System(system_), stringify(
                    "can't rename ", tempfile_path_.c_str(),
                    " to ", path_.c_str(), ": ", strerror(errno)));
            }
        }
    }
}

Output_File::~Output_File()
{
    remove(tempfile_path_.c_str());
}

} // namespace
