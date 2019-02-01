// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

extern "C" {
#include <unistd.h>
}
#include <iostream>

#include <libcurv/context.h>
#include <libcurv/json.h>
#include <libcurv/progdir.h>
#include <libcurv/program.h>
#include <libcurv/shape.h>
#include <libcurv/source.h>
#include <libcurv/system.h>
#include <libcurv/viewed_shape.h>

namespace fs = curv::Filesystem;
using namespace curv;

int
main(int argc, char** argv)
{
    if (argc != 2) {
        std::cerr << "Usage: curvc filename\n";
        return EXIT_FAILURE;
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
        Shape_Program shape{prog};
        if (!shape.recognize(value)) {
            std::cout << "{\"value\":";
            write_json_value(value, std::cout);
            std::cout << "}\n";
        } else {
            Frag_Export opts;
            Viewed_Shape vshape(shape, opts);
            std::cout << "{\"shape\":{"
                << "\"is_2d\":" << Value{shape.is_2d_}
                << ",\"is_3d\":" << Value{shape.is_3d_}
                << ",\"bbox\":[[" << dfmt(shape.bbox_.xmin, dfmt::JSON)
                    << "," << dfmt(shape.bbox_.ymin, dfmt::JSON)
                    << "," << dfmt(shape.bbox_.zmin, dfmt::JSON)
                    << "],[" << dfmt(shape.bbox_.xmax, dfmt::JSON)
                    << "," << dfmt(shape.bbox_.ymax, dfmt::JSON)
                    << "," << dfmt(shape.bbox_.zmax, dfmt::JSON)
                << "]],";
            vshape.write_json(std::cout);
            std::cout << "}}\n";
        }
    } catch (std::exception& e) {
        sys.error(e);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
