// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#ifndef LIBCURV_GEOM_CPP_PROGRAM_H
#define LIBCURV_GEOM_CPP_PROGRAM_H

#include <libcurv/filesystem.h>
#include <libcurv/sc_compiler.h>
#include <libcurv/system.h>
#include <fstream>

namespace curv { namespace geom {

// A structure for building a C++ source file, compiling it, and getting
// the results. This holds the C++ source code and the compiled binary.
struct Cpp_Program
{
    System& system_;
    unsigned tempfile_id_;
    Filesystem::path path_;
    std::ofstream file_;
    SC_Compiler sc_;
    void* dll_ = nullptr;

    Cpp_Program(System&);
    ~Cpp_Program();
    static const char standard_header[];
    inline void define_function(
        const char* name, SC_Type param_type, SC_Type result_type,
        Shared<const Function> func, const Context& cx)
    {
        sc_.define_function(name, param_type, result_type, func, cx);
    }
    void compile(const Context& cx);
    void* get_function(const char* name);
    void preserve_tempfile();
};

}} // namespace
#endif // include guard
