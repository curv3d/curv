// Copyright 2016-2021 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/cpp_program.h>

#include <libcurv/geom/tempfile.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>

// Functions to load shared libraries
#ifdef _WIN32
    #include <libcurv/win32.h>
#else
    extern "C" {
    #include <dlfcn.h>
    }
#endif

#include <iostream>

// TODO: Add Windows support by means of LoadLibrary() and friends

namespace curv { namespace geom {

const char Cpp_Program::standard_header[] =
    "#include <glm/common.hpp>\n"
    "#include <glm/matrix.hpp>\n"
    "#include <glm/geometric.hpp>\n"
    "#include <glm/trigonometric.hpp>\n"
    "#include <glm/exponential.hpp>\n"
    "\n"
    "using namespace glm;\n"
    "\n";

Cpp_Program::Cpp_Program(Source_State& ss)
:
    sstate_{ss},
    tempfile_id_{make_tempfile_id()},
    path_{register_tempfile(tempfile_id_,".cpp")},
    file_{path_.c_str()},
    sc_{file_, SC_Target::cpp, ss}
{
    if (file_.fail()) {
        throw Exception{At_SState{ss},
            stringify("cannot open file ",path_.string())};
    }
    file_ << standard_header;
}

Cpp_Program::~Cpp_Program()
{
#ifdef _WIN32
    if (dll_ != NULL)
        FreeLibrary(dll_);
#else
    if (dll_ != nullptr)
        dlclose(dll_);
#endif
}

void
Cpp_Program::compile(const Context& cx)
{
    file_.close();

    // compile C++ to optimized object code
    auto cc_cmd = stringify("c++ -fpic -O3 -c ", path_.string());
    //auto cc_cmd = stringify("c++ -fpic -c -g ", path_.c_str());
    if (system(cc_cmd->c_str()) != 0) {
        preserve_tempfile();
        throw Exception(cx, stringify("c++ compile failed; see ", path_));
    }

    // create shared object
    auto obj_name = register_tempfile(tempfile_id_,".o");
#ifdef _WIN32
    auto lib_name = register_tempfile(tempfile_id_,".dll");
#else
    auto lib_name = register_tempfile(tempfile_id_,".so");
#endif
    auto link_cmd = stringify("c++ -shared -o ", lib_name.string(), " ", obj_name.string());
    if (system(link_cmd->c_str()) != 0)
        throw Exception(cx, "c++ link failed");

    // load shared object
#ifdef _WIN32
    dll_ = LoadLibraryW(lib_name.c_str()); // use ANSI variant to avoid the need to convert char* to wchar_t*
    if (dll_ == NULL)
    {
        DWORD error = GetLastError();
        throw Exception(cx, stringify("can't load shared object: ", win_strerror(error)));
    }
#else
    // TODO: lib_name should contain a / character to prevent PATH search.
    // On macOS with a code-signed curv executable, so_name may need to be an
    // absolute pathname.
    dll_ = dlopen(lib_name.c_str(), RTLD_NOW|RTLD_LOCAL);
    if (dll_ == nullptr)
        throw Exception(cx, stringify("can't load shared object: ", dlerror()));
#endif
}

void*
Cpp_Program::get_function(const char* name)
{
#ifdef _WIN32
    void* function = (void*) GetProcAddress(dll_, name);
    if (!function)
    {
        DWORD error = GetLastError();
        throw Exception(At_SState{sstate_},
            stringify("can't load function ", name, ": ", win_strerror(error)));
    }
    return function;
#else
    dlerror(); // Clear previous error.
    void* function = dlsym(dll_, name);
    const char* err = dlerror();
    if (err != nullptr) {
        throw Exception(At_SState{sstate_},
            stringify("can't load function ", name, ": ", err));
    }
    if (function == nullptr) {
        throw Exception(At_SState{sstate_},
            stringify("can't load function ", name, ": got null pointer"));
    }
    return function;
#endif
}

void
Cpp_Program::preserve_tempfile()
{
    deregister_tempfile(path_);
}

}} // namespace
