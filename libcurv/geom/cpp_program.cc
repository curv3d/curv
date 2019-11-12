// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

#include <libcurv/geom/cpp_program.h>

#include <libcurv/geom/tempfile.h>
#include <libcurv/context.h>
#include <libcurv/exception.h>
extern "C" {
#include <dlfcn.h>
}
#include <iostream>

namespace curv { namespace geom {

const char Cpp_Program::standard_header[] =
    "#include <glm/vec2.hpp>\n"
    "#include <glm/vec3.hpp>\n"
    "#include <glm/vec4.hpp>\n"
    "#include <glm/common.hpp>\n"
    "#include <glm/geometric.hpp>\n"
    "#include <glm/trigonometric.hpp>\n"
    "#include <glm/exponential.hpp>\n"
    "\n"
    "using namespace glm;\n"
    "\n";

Cpp_Program::Cpp_Program(System& sys)
:
    system_{sys},
    tempfile_id_{make_tempfile_id()},
    path_{register_tempfile(tempfile_id_,".cpp")},
    file_{path_.c_str()},
    sc_{file_, SC_Target::cpp, sys}
{
    if (file_.fail()) {
        throw Exception{At_System{system_},
            stringify("cannot open file ",path_.c_str())};
    }
    file_ << standard_header;
}

Cpp_Program::~Cpp_Program()
{
    if (dll_ != nullptr)
        dlclose(dll_);
}

void
Cpp_Program::compile(const Context& cx)
{
    file_.close();

    // compile C++ to optimized object code
    auto cc_cmd = stringify("c++ -fpic -O3 -c ", path_.c_str());
    //auto cc_cmd = stringify("c++ -fpic -c -g ", path_.c_str());
    if (system(cc_cmd->c_str()) != 0) {
        preserve_tempfile();
        throw Exception(cx, stringify("c++ compile failed; see ", path_));
    }

    // create shared object
    auto obj_name = register_tempfile(tempfile_id_,".o");
    auto so_name = register_tempfile(tempfile_id_,".so");
    auto link_cmd = stringify("c++ -shared -o ", so_name.c_str(), " ", obj_name.c_str());
    if (system(link_cmd->c_str()) != 0)
        throw Exception(cx, "c++ link failed");

    // load shared object
    // TODO: so_name should contain a / character to prevent PATH search.
    // On macOS with a code-signed curv executable, so_name may need to be an
    // absolute pathname.
    dll_ = dlopen(so_name.c_str(), RTLD_NOW|RTLD_LOCAL);
    if (dll_ == nullptr)
        throw Exception(cx, stringify("can't load shared object: ", dlerror()));
}

void*
Cpp_Program::get_function(const char* name)
{
    dlerror(); // Clear previous error.
    void* object = dlsym(dll_, name);
    const char* err = dlerror();
    if (err != nullptr) {
        throw Exception(At_System{system_},
            stringify("can't load function ",name,": ",err));
    }
    if (object == nullptr) {
        throw Exception(At_System{system_},
            stringify("can't load function ",name,": got null pointer"));
    }
    return object;
}

void
Cpp_Program::preserve_tempfile()
{
    deregister_tempfile(path_);
}

}} // namespace
