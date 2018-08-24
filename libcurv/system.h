// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_SYSTEM_H
#define LIBCURV_SYSTEM_H

#include <ostream>
#include <unordered_set>
#include <map>
#include <libcurv/filesystem.h>
#include <libcurv/builtin.h>

namespace curv {

struct Context;

/// An abstract interface to the client and operating system.
///
/// The System object is owned by the client, who is responsible for ensuring
/// that it exists for as long as references to it might exist in Curv
/// data structures.
struct System
{
    /// This is the set of standard or builtin bindings
    /// used by the `file` primitive to interpret Curv source files.
    virtual const Namespace& std_namespace() = 0;

    // Set to true if you want coloured text to be written on the console.
    bool use_colour_ = false;

    virtual std::ostream& console() = 0;

    // Write an error message on the console, given an exception object.
    // Special behaviour for curv::Exception. Honours `use_colour_`.
    void message(const char* prefix, const std::exception&);

    // This is non-empty while a `file` operation is being evaluated.
    // It is used to detect recursive file references.
    // Later, this may change to a file cache.
    // Or, it could move into the Program structure.
    std::unordered_set<Filesystem::path,Path_Hash> active_files_{};

    // Used by `file` to import a file based on its extension.
    // The extension includes the leading '.', and "" means no extension.
    // The extension is converted to lowercase on all platforms.
    using Importer = Value (*)(const Filesystem::path&, const Context&);
    std::map<std::string,Importer> importers_;
};

// RAII helper class, for use with System::active_files_.
struct Active_File
{
    std::unordered_set<Filesystem::path,Path_Hash>& active_files_;
    Filesystem::path& file_;
    Active_File(
        std::unordered_set<Filesystem::path,Path_Hash>& af,
        Filesystem::path& f)
    :
        active_files_(af),
        file_(f)
    {
        active_files_.insert(file_);
    }
    ~Active_File()
    {
        active_files_.erase(file_);
    }
};

/// Default implementation of the System interface.
struct System_Impl : public System
{
    Namespace std_namespace_;
    std::ostream& console_;
    System_Impl(std::ostream&);
    void load_library(String_Ref path);
    virtual const Namespace& std_namespace() override;
    virtual std::ostream& console() override;
};

} // namespace curv
#endif // header guard
