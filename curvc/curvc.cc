// Copyright 2016-2019 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

extern "C" {
#include <unistd.h>
}
#include <iostream>

#include <libcurv/context.h>
#include <libcurv/gpu_program.h>
#include <libcurv/json.h>
#include <libcurv/progdir.h>
#include <libcurv/program.h>
#include <libcurv/source.h>
#include <libcurv/system.h>
#include <libcurv/version.h>

namespace fs = curv::Filesystem;
using namespace curv;

int
main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: curvc filename | curvc --version\n";
        return EXIT_FAILURE;
    }
    if (strcmp(argv[1], "--version") == 0) {
        std::cout << CURV_VERSION << "\n";
        return EXIT_SUCCESS;
    }
    System_Impl sys(std::cout);
    sys.use_json_api_ = true;
    try {
        sys.load_library(
            fs::canonical(progdir(argv[0])/"../lib/std.curv").c_str());
        auto source = make<File_Source>(argv[1], At_System(sys));
        Program prog{std::move(source), sys};
        prog.compile();
        auto value = prog.eval();
        GPU_Program gprog{prog};
        Frag_Export opts;
        if (!gprog.recognize(value, opts)) {
            std::cout << "{\"value\":";
            write_json_value(value, std::cout);
            std::cout << "}\n";
        } else {
            std::cout << "{\"shape\":";
            gprog.write_json(std::cout);
            std::cout << "}\n";
        }
    } catch (std::exception& e) {
        sys.error(e);
    }
    return EXIT_SUCCESS;
}
