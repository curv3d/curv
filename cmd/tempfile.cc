// Copyright 2016-2018 Doug Moen
// Licensed under the Apache Licence, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#include "tempfile.h"
#include <libcurv/exception.h>
#include <libcurv/context.h>
#include <vector>

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
}

namespace fs = boost::filesystem;

std::vector<fs::path> tempfiles;

fs::path
tempfile_name(const char* suffix)
{
    auto name = curv::stringify(",curv",getpid(),suffix);
    return fs::current_path() / fs::path(name->c_str());
}

fs::path
make_tempfile(const char* suffix)
{
    auto filename = tempfile_name(suffix);
    int fd = creat(filename.c_str(), 0666);
    if (fd == -1)
        throw curv::Exception({}, curv::stringify(
            "Can't create ",filename.c_str(),": ",strerror(errno)));
    close(fd);
    tempfiles.push_back(filename);
    return filename;
}

void
register_tempfile(const char* suffix)
{
    auto filename = tempfile_name(suffix);
    tempfiles.push_back(filename);
}

void
remove_all_tempfiles()
{
    for (auto& file : tempfiles)
        remove(file.c_str());
}
