// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

extern "C" {
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
}
#include <iostream>
#include <fstream>

#include "export.h"
#include "progdir.h"
#include "repl.h"
#include "cscript.h"
#include "shapes.h"
#include "livemode.h"
#include "version.h"

#include <libcurv/geom/tempfile.h>
#include <libcurv/dtostr.h>
#include <libcurv/analyser.h>
#include <libcurv/context.h>
#include <libcurv/program.h>
#include <libcurv/exception.h>
#include <libcurv/file.h>
#include <libcurv/parser.h>
#include <libcurv/phrase.h>
#include <libcurv/shared.h>
#include <libcurv/system.h>
#include <libcurv/list.h>
#include <libcurv/record.h>
#include <libcurv/die.h>
#include <libcurv/geom/export_frag.h>
#include <libcurv/geom/import.h>
#include <libcurv/geom/shape.h>
#include <libcurv/geom/viewer/viewer.h>

curv::System&
make_system(const char* argv0, std::list<const char*>& libs)
{
    try {
        static curv::System_Impl sys(std::cerr);
        if (argv0 != nullptr) {
            const char* CURV_STDLIB = getenv("CURV_STDLIB");
            namespace fs = boost::filesystem;
            curv::Shared<const curv::String> stdlib;
            if (CURV_STDLIB != nullptr) {
                if (CURV_STDLIB[0] != '\0')
                    stdlib = curv::make_string(CURV_STDLIB);
                else
                    stdlib = nullptr;
            } else {
                fs::path stdlib_path =
                    fs::canonical(progdir(argv0) / "../lib/std.curv");
                stdlib = curv::make_string(stdlib_path.c_str());
            }
            sys.load_library(stdlib);
        }
        for (const char* lib : libs) {
            sys.load_library(curv::make_string(lib));
        }
        curv::geom::add_importers(sys);
        return sys;
    } catch (curv::Exception& e) {
        std::cerr << "ERROR: " << e << "\n";
        exit(EXIT_FAILURE);
    } catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}

const char help[] =
"curv [options] [filename]\n"
"-n -- don't use standard library\n"
"-i file -- include specified library; may be repeated\n"
"-l -- live programming mode\n"
"-e -- run <$CURV_EDITOR filename> in live mode\n"
"-x -- interpret filename argument as expression\n"
"-o format -- output format:\n"
"   curv -- Curv expression\n"
"   json -- JSON expression\n"
"   frag -- GLSL fragment shader (shape only, shadertoy.com compatible)\n"
"   stl -- STL mesh file (3D shape only)\n"
"   obj -- OBJ mesh file (3D shape only)\n"
"   x3d -- X3D colour mesh file (3D shape only)\n"
"   cpp -- C++ source file (shape only)\n"
"-O name=value -- parameter for one of the output formats\n"
"--version -- display version information needed for bug reports.\n"
"--help -- display this help information.\n"
"filename -- input file, a Curv script. Interactive CLI if missing.\n"
;

int
main(int argc, char** argv)
{
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        std::cout << help;
        return EXIT_SUCCESS;
    }
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        print_version(std::cout);
        return EXIT_SUCCESS;
    }

    // Parse arguments.
    const char* argv0 = argv[0];
    const char* usestdlib = argv0;
    void (*exporter)(curv::Value,
        curv::Program&,
        const Export_Params&,
        std::ostream&)
        = nullptr;
    Export_Params eparams;
    bool live = false;
    std::list<const char*> libs;
    bool expr = false;
    const char* editor = nullptr;

    int opt;
    while ((opt = getopt(argc, argv, ":o:O:lni:xe")) != -1) {
        switch (opt) {
        case 'o':
            if (strcmp(optarg, "curv") == 0)
                exporter = export_curv;
            else if (strcmp(optarg, "json") == 0)
                exporter = export_json;
            else if (strcmp(optarg, "frag") == 0)
                exporter = export_frag;
            else if (strcmp(optarg, "stl") == 0)
                exporter = export_stl;
            else if (strcmp(optarg, "obj") == 0)
                exporter = export_obj;
            else if (strcmp(optarg, "x3d") == 0)
                exporter = export_x3d;
            else if (strcmp(optarg, "cpp") == 0)
                exporter = export_cpp;
            else {
                std::cerr << "-o: format " << optarg << " not supported\n"
                          << "Use " << argv0 << " --help for help.\n";
                return EXIT_FAILURE;
            }
            break;
        case 'O':
          {
            char* eq = strchr(optarg, '=');
            if (eq == nullptr) {
                eparams[std::string(optarg)] = std::string("");
            } else {
                *eq = '\0';
                eparams[std::string(optarg)] = std::string(eq+1);
                *eq = '=';
            }
            break;
          }
        case 'l':
            live = true;
            break;
        case 'n':
            usestdlib = nullptr;
            break;
        case 'i':
            libs.push_back(optarg);
            break;
        case 'x':
            expr = true;
            break;
        case 'e':
            editor = getenv("CURV_EDITOR");
            if (editor == nullptr)
                editor = "gedit --new-window --wait";
            break;
        case '?':
            std::cerr << "-" << (char)optopt << ": unknown option\n"
                     << "Use " << argv0 << " --help for help.\n";
            return EXIT_FAILURE;
        case ':':
            std::cerr << "-" << (char)optopt << ": missing argument\n"
                     << "Use " << argv0 << " --help for help.\n";
            return EXIT_FAILURE;
        default:
            curv::die("main: bad result from getopt()");
        }
    }
    const char* filename;
    if (optind >= argc) {
        filename = nullptr;
    } else if (argc - optind > 1) {
        std::cerr << "too many filename arguments\n"
                  << "Use " << argv0 << " --help for help.\n";
        return EXIT_FAILURE;
    } else
        filename = argv[optind];

    // Validate arguments
    if (live) {
        if (exporter) {
            std::cerr << "-l and -o flags are not compatible.\n"
                      << "Use " << argv0 << " --help for help.\n";
            return EXIT_FAILURE;
        }
    }
    if (filename == nullptr) {
        if (expr) {
            std::cerr << "missing expression argument\n"
                      << "Use " << argv0 << " --help for help.\n";
            return EXIT_FAILURE;
        }
        if (exporter != nullptr || live) {
            std::cerr << "missing filename argument\n"
                      << "Use " << argv0 << " --help for help.\n";
            return EXIT_FAILURE;
        }
    }
    if (editor && !live) {
        std::cerr << "-e flag specified without -l flag.\n"
                  << "Use " << argv0 << " --help for help.\n";
        return EXIT_FAILURE;
    }

    // Interpret arguments
    curv::System& sys(make_system(usestdlib, libs));
    atexit(curv::geom::remove_all_tempfiles);

    if (filename == nullptr) {
        interactive_mode(sys);
        return EXIT_SUCCESS;
    }

    if (live) {
        return live_mode(sys, editor, filename);
    }

    // batch mode
    try {
        curv::Shared<curv::Script> script;
        if (expr) {
            script = curv::make<CString_Script>("", filename);
        } else {
            script = curv::make<curv::File_Script>(
                curv::make_string(filename), curv::Context{});
        }

        curv::Program prog{*script, sys};
        prog.compile();
        auto value = prog.eval();

        if (exporter == nullptr) {
            curv::geom::Shape_Program shape{prog};
            if (shape.recognize(value)) {
                print_shape(shape);
                curv::geom::viewer::Viewer viewer;
                viewer.set_shape(shape);
                viewer.run();
            } else {
                std::cout << value << "\n";
            }
        } else {
            exporter(value, prog, eparams, std::cout);
        }
    } catch (curv::Exception& e) {
        std::cerr << "ERROR: " << e << "\n";
        return EXIT_FAILURE;
    } catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
