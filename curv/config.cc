// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include "config.h"

#include <libcurv/exception.h>
#include <libcurv/filesystem.h>
#include <libcurv/import.h>
#include <iostream>
extern "C" {
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
}

using namespace curv;
namespace fs = curv::Filesystem;

Shared<const Record>
load_config(const fs::path& name, const Context& cx)
{
    int r = access(name.c_str(), R_OK);
    if (r == 0) {
        Value val = import(name, cx);
        auto config = val.dycast<const Record>();
        if (config == nullptr)
            throw Exception(cx,
                stringify(name, ": config data is not a record value"));
        return config;
    }
    if (errno == ENOENT)
        return nullptr;
    throw Exception(cx, stringify(name, ": ", strerror(errno)));
}

// Find the user's Curv config file, evaluate it, and return the config value.
// If the config file cannot be found, return missing.
// If an error occurs during processing (or an access error occurs while
// opening the file), then throw an exception.
Shared<const Record>
get_config(const Context& cx, Shared<const String>&cpath_out)
{
    fs::path config_dir;
    const char* XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
    if (XDG_CONFIG_HOME == nullptr || XDG_CONFIG_HOME[0] == '\0') {
        const char* HOME = getenv("HOME");
        if (HOME == nullptr || HOME[0] == '\0')
            return nullptr;
        config_dir = HOME;
        config_dir /= ".config";
    } else {
        config_dir = XDG_CONFIG_HOME;
    }
    fs::path cpath = config_dir / "curv.curv";
    auto config = load_config(cpath, cx);
    if (config != nullptr) {
        cpath_out = make_string(cpath.c_str());
        return config;
    }
    cpath = config_dir / "curv";
    config = load_config(cpath, cx);
    if (config != nullptr) {
        cpath_out = make_string(cpath.c_str());
        return config;
    }
    return nullptr;
}
