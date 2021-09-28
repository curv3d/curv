// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef CURV_CONFIG_H
#define CURV_CONFIG_H

#include <libcurv/context.h>
#include <libcurv/record.h>

struct Config {
    Config() {}
    Config(
        curv::Shared<const curv::Record> root,
        curv::Shared<const curv::Record> branch,
        curv::String_Ref filename,
        curv::Symbol_Ref branchname)
    :
        root_(root),
        branch_(branch),
        filename_(filename),
        branchname_(branchname)
    {}
    bool is_empty() const { return root_ == nullptr; }
    curv::Shared<const curv::Record> root_;
    curv::Shared<const curv::Record> branch_;
    curv::Shared<const curv::String> filename_;
    curv::Symbol_Ref branchname_;
};

Config get_config(curv::System&, curv::Symbol_Ref);

#endif // header guard
