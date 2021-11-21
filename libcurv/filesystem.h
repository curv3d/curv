// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_FILESYSTEM_H
#define LIBCURV_FILESYSTEM_H

#include <filesystem>
#include <functional>
#include <string>

namespace curv {

namespace Filesystem = std::filesystem;

struct Path_Hash
{
    std::size_t operator()(Filesystem::path const& p) const noexcept
    {
        return std::hash<std::string>{}(p.string());
    }
};

} // namespace curv
#endif // header guard
