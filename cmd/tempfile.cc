// Copyright 2016-2018 Doug Moen
// Licensed under the Apache Licence, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

#include "tempfile.h"
#include <curv/exception.h>
#include <curv/context.h>

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
}

curv::Shared<curv::String> tempfile = nullptr;

curv::Shared<curv::String>
make_tempfile(const char* suffix)
{
    auto filename = curv::stringify(",curv",getpid(),suffix);
    int fd = creat(filename->c_str(), 0666);
    if (fd == -1)
        throw curv::Exception({}, curv::stringify(
            "Can't create ",filename->c_str(),": ",strerror(errno)));
    close(fd);
    tempfile = filename;
    return filename;
}

void
remove_all_tempfiles()
{
    if (tempfile != nullptr)
        remove(tempfile->c_str());
}
