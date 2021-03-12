// Copyright 2016-2020 Doug Moen
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

struct At_Path : public Context
{
    System& sys_;
    fs::path& path_;

    At_Path(System& sys, fs::path& path)
    :
        sys_(sys),
        path_(path)
    {}

    virtual void get_locations(std::list<Func_Loc>& locs) const override
    {
        locs.emplace_back(
            Src_Loc(make<Source>(path_.string().c_str()), Token{}));
    }
    virtual System& system() const override
    {
        return sys_;
    }
    virtual Frame* frame() const override
    {
        return nullptr;
    }
};

Shared<const Record>
load_config(const At_Path& cx)
{
    int r = access(cx.path_.string().c_str(), R_OK);
    if (r == 0)
        return import(cx.path_, cx).to<Record>(cx);
    if (errno == ENOENT)
        return nullptr;
    throw Exception(cx, strerror(errno));
}

// Find the user's Curv config file, evaluate it, and return the config value.
// If the config file cannot be found, return missing.
// If an error occurs during processing (or an access error occurs while
// opening the file), then throw an exception.
Config
get_config(System& sys, Symbol_Ref branchname)
{
    fs::path config_dir;
    const char* XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
    if (XDG_CONFIG_HOME == nullptr || XDG_CONFIG_HOME[0] == '\0') {
        const char* HOME = getenv("HOME");
        if (HOME == nullptr || HOME[0] == '\0')
            return Config{};
        config_dir = HOME;
        config_dir /= ".config";
    } else {
        config_dir = XDG_CONFIG_HOME;
    }
  #if 0
    // I changed my mind about supporting multiple config file names.
    fs::path cpath = config_dir / "curv.curv";
    auto config = load_config(cpath, cx);
    if (config != nullptr) {
        cpath_out = make_string(cpath.string().c_str());
        return config;
    }
  #endif
    auto cpath = config_dir / "curv";
    At_Path cx(sys, cpath);
    auto root = load_config(cx);
    if (root != nullptr) {
        Value branchval = root->find_field(branchname, cx);
        if (!branchval.is_missing()) {
            auto branch = branchval.to<Record>(At_Field(branchname.c_str(), cx));
            return Config{root, branch, cpath.string(), branchname};
        }
    }
    return Config{};
}
