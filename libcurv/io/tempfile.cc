// Copyright 2016-2021 Doug Moen
// Licensed under the Apache Licence, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/io/tempfile.h>
#include <libcurv/exception.h>
#include <libcurv/context.h>
#include <algorithm>
#include <boost/filesystem.hpp>
#include <iostream>
#include <vector>


extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
}

namespace curv { namespace io {

namespace fs = Filesystem;

std::vector<fs::path> tempfiles;
unsigned tempfile_id = 0;

unsigned
make_tempfile_id()
{
    return tempfile_id++;
}

fs::path
tempfile_name(unsigned id, const char* suffix)
{
    auto name = stringify(",curv-",getpid(),"-",id,suffix);
    return fs::current_path() / fs::path(name->c_str());
}

fs::path
register_tempfile(unsigned id, const char* suffix)
{
    auto filename = tempfile_name(id, suffix);
    tempfiles.push_back(filename);
    return filename;
}

void
deregister_tempfile(fs::path name)
{
    auto p = std::find(tempfiles.begin(), tempfiles.end(), name);
    if (p != tempfiles.end())
        tempfiles.erase(p);
}

void
remove_all_tempfiles()
{
    for (auto& file : tempfiles)
    {
        boost::system::error_code error;
        fs::remove(file, error);
        if (error)
        {
            std::cerr << "Error while removing temporary file: " << error.message() << "\nIgnoring...\n";
        }
    }
}

}} // namespace
