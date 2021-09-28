// Copyright 2016-2021 Doug Moen
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
struct Program;

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

    // Set by the `-v` command line argument.
    bool verbose_ = false;

    // Deprecation level (0, 1 or 2) for deprecation warnings.
    // Set by the `--depr=N` command line argument.
    int depr_ = 1;

    // Set to true if you want coloured text to be written on the console.
    bool use_colour_ = false;

    // True if the json-api protocol is being used.
    bool use_json_api_ = false;

    virtual std::ostream& console() = 0;

    // Write an exception object to an output stream, using the Curv colour
    // scheme if use_colour is true. Special behaviour for curv::Exception.
    // This static function is handy for printing an exception caught during
    // the construction of a System object (otherwise, use System::message).
    static void print_exception(
        const char* prefix, const std::exception& exc,
        std::ostream& out, bool use_colour = false);

    static void print_json_exception(
        const char* type, const std::exception& exc, std::ostream& out);

    void error(const std::exception& exc);
    void warning(const std::exception& exc);
    void print(const char*);

    // This is non-empty while a `file` operation is being evaluated.
    // It is used to detect recursive file references.
    // Later, this may change to a file cache.
    // Or, it could move into the Program structure.
    std::unordered_set<Filesystem::path,Path_Hash> active_files_{};

    // Used by `file` to import a file based on its extension.
    // The extension includes the leading '.', and "" means no extension.
    // The extension is converted to lowercase on all platforms.
    using Importer = void (*)(const Filesystem::path&, Program&, const Context&);
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
