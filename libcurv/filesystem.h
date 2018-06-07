// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_FILESYSTEM_H
#define LIBCURV_FILESYSTEM_H

#include <boost/filesystem.hpp>
#include <functional>
#include <string>

namespace curv {

// Once we migrate to C++17, we'll use std::filesystem instead.
namespace Filesystem = boost::filesystem;

struct Path_Hash
{
    std::size_t operator()(Filesystem::path const& p) const noexcept
    {
        return std::hash<std::string>{}(p.string());
    }
};

} // namespace curv
#endif // header guard
