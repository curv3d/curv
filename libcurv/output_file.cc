// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/output_file.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>
#include <boost/filesystem.hpp>

#ifdef _WIN32
    #include <libcurv/win32.h>
#endif

extern "C" {
#include <fcntl.h>
#include <time.h>
#include <unistd.h>

#ifndef _WIN32
    #include <sys/types.h>
    #include <sys/stat.h>
#endif
}

#include <iostream>
#include <fstream>
#include <string>

namespace curv {

namespace fs = Filesystem;
namespace io = boost::iostreams;

// Create a uniquely-named tempfile in the specified directory.
// The tempfile name ends in `suffix`.
// The file will be created and exclusively opened, preventing race conditions.
// The file name is stored in `path`.
// The open file descriptor is stored in `stream`.
// An exception is thrown on error.
io::file_descriptor_sink::handle_type
maketemp(
    const fs::path& tempdir,
    const std::string& suffix,
    fs::path& path,
    System& sys)
{
    pid_t pid = getpid();
    fs::path trypath;
    for (int i = 0; i < 20; ++i) {
        // TODO: why not use Boost's function to generate unique filenames?
        clock_t now = clock();
        auto name = stringify(",curv-",pid,"-",now,suffix);
        trypath = tempdir / fs::path(name->c_str());
#ifdef _WIN32
        HANDLE fd = CreateFileW(trypath.c_str(), GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY, NULL);
        if (fd == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            if (error == ERROR_FILE_EXISTS)
            {
                continue;
            }

            throw Exception(At_System(sys), stringify("can't create tempfile ", trypath.c_str(), ": ", win_strerror(error).c_str()));
        } else {
            path = trypath;
            return fd;
        }
#else
        int fd = open(trypath.c_str(), O_WRONLY|O_CREAT|O_EXCL, 0600);
        if (fd != -1) {
            path = trypath;
            return fd;
        } else if (errno == EEXIST) {
            continue;
        } else {
            const char *error_msg = strerror(errno);
            throw Exception(At_System(sys), stringify("can't create tempfile ",trypath.c_str(), ": ", error_msg));
        }
#endif
    }

    throw Exception(At_System(sys), stringify("can't choose unique name to create tempfile, last name was: ", trypath.c_str()));
}

void
Output_File::open()
{
    fs::path tempdir;
    if (path_.empty())
        tempdir = fs::temp_directory_path();
    else
        tempdir = path_.parent_path();
    io::file_descriptor_sink::handle_type fd = maketemp(tempdir, ".tmp", tempfile_path_, system_);
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
            try
            {
                fs::rename(tempfile_path_, path_);
            }
            catch (fs::filesystem_error &error)
            {
                throw Exception(At_System(system_), stringify(
                    "can't rename ", tempfile_path_.string().c_str(),
                    " to ", path_.string().c_str(), ": ", error.what()));
            }
        }
    }
}

Output_File::~Output_File()
{
    boost::system::error_code error;
    fs::remove(tempfile_path_, error);
    if (error)
    {
        std::cerr << "Error while removing temporary output file: " << error.message() << "\nIgnoring...\n";
    }
}

} // namespace
