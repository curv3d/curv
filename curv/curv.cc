// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

extern "C" {
#include <unistd.h>
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

#include <libcurv/context.h>
#include <libcurv/die.h>
#include <libcurv/exception.h>
#include <libcurv/file.h>
#include <libcurv/output_file.h>
#include <libcurv/program.h>
#include <libcurv/system.h>
#include <libcurv/geom/import.h>
#include <libcurv/geom/shape.h>
#include <libcurv/geom/tempfile.h>
#include <libcurv/geom/viewer/viewer.h>

namespace fs = curv::Filesystem;

curv::System&
make_system(const char* argv0, std::list<const char*>& libs)
{
    static curv::System_Impl sys(std::cerr);
    if (isatty(2)) sys.use_colour_ = true;
    try {
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
    } catch (std::exception& e) {
        sys.message("ERROR: ", e);
        exit(EXIT_FAILURE);
    }
}

const char help_prefix[] =
"curv --help [-o format]\n"
"   Display help information.\n"
"   -o format : Display help for an output format, showing -O parameters.\n"
"curv --version\n"
"   Display version information needed for bug reports.\n"
"curv [options]\n"
"   Interactive shell.\n"
"curv -l [-e] [options] filename\n"
"   Live programming mode. Evaluate & display result each time file changes.\n"
"   -e : Open editor window. $CURV_EDITOR overrides default editor.\n"
"curv [-o arg] [-O arg]... [-x] [options] filename\n"
"   Batch mode. Evaluate file, display result or export to a file.\n"
"   -o format : Convert to specified file format, write data to stdout.\n"
;

const char help_suffix[] =
"   -o filename.ext : Export to file, using filename extension as format.\n"
"   -O name=value : Parameter for the specified output format.\n"
"   -x : Interpret filename argument as expression.\n"
"general options:\n"
"   $CURV_STDLIB : Pathname of standard library, overrides PREFIX/lib/std.curv\n"
"   -n : Don't use standard library.\n"
"   -i file : Include specified library; may be repeated.\n"
;

int
main(int argc, char** argv)
{
    const char* argv0 = argv[0];

    // Handle 'curv --help'.
    if (argc >= 2 && strcmp(argv[1], "--help") == 0) {
        if (argc == 4 && strcmp(argv[2], "-o") == 0) {
            const char* format = argv[3];
            auto ex = exporters.find(format);
            if (ex == exporters.end()) {
                std::cerr << "-o: format '" << format << "' not supported.\n"
                          << "Use " << argv0 << " --help for help.\n";
                return EXIT_FAILURE;
            }
            std::cout << ex->second.synopsis << "\n" << ex->second.description;
            return EXIT_SUCCESS;
        }
        if (argc == 2) {
            std::cout << help_prefix;
            for (auto& ex : exporters) {
                std::cout << "      " << ex.first << " : "
                          << ex.second.synopsis << "\n";
            }
            std::cout << help_suffix;
            return EXIT_SUCCESS;
        }
        std::cerr << argv[2] << ": bad argument to --help\n"
                  << "Use " << argv0 << " --help for help.\n";
        return EXIT_FAILURE;
    }

    // Handle 'curv --version'.
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        print_version(std::cout);
        return EXIT_SUCCESS;
    }

    // Parse arguments for general case.
    const char* usestdlib = argv0;
    Exporter* exporter = nullptr;
    Export_Params eparams;
    curv::Output_File ofile;
    bool live = false;
    std::list<const char*> libs;
    bool expr = false;
    const char* editor = nullptr;

    int opt;
    while ((opt = getopt(argc, argv, ":o:O:lni:xe")) != -1) {
        switch (opt) {
        case 'o':
          {
            const char* oarg = optarg;
            fs::path opath{oarg};
            std::string ext_string = opath.extension().string();
            const char* ext = ext_string.c_str();
            const char* oname;
            if (ext[0] == '.')
                oname = &ext[1];
            else
                oname = oarg;
            auto ex = exporters.find(oname);
            if (ex == exporters.end()) {
                std::cerr << "-o: format '" << oname << "' not supported.\n"
                          << "Use " << argv0 << " --help for help.\n";
                return EXIT_FAILURE;
            }
            exporter = &ex->second;
            if (oname == oarg)
                ofile.set_ostream(&std::cout);
            else
                ofile.set_path(opath);
            eparams.format = oname;
            break;
          }
        case 'O':
          {
            char* eq = strchr(optarg, '=');
            if (eq == nullptr) {
                eparams.map[std::string(optarg)] = std::string("");
            } else {
                *eq = '\0';
                eparams.map[std::string(optarg)] = std::string(eq+1);
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
        if (expr) {
            std::cerr << "-l and -x flags are not compatible.\n"
                      << "Use " << argv0 << " --help for help.\n";
            return EXIT_FAILURE;
        }
    }
    if (filename == nullptr) {
        if (expr || exporter || live) {
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
    if (!eparams.map.empty() && !exporter) {
        std::cerr << "-O flag specified without -o flag.\n"
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

        if (exporter) {
            exporter->call(value, prog, eparams, ofile);
            ofile.commit();
        } else {
            curv::geom::Shape_Program shape{prog};
            if (shape.recognize(value)) {
                print_shape(shape);
                curv::geom::viewer::Viewer viewer;
                curv::geom::Frag_Export opts;
                viewer.set_shape(shape, opts);
                viewer.run();
            } else {
                std::cout << value << "\n";
            }
        }
    } catch (std::exception& e) {
        sys.message("ERROR: ", e);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
