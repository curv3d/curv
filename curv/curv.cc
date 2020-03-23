// Copyright 2016-2020 Doug Moen
// Licensed under the Apache License, version 2.0
// See accompanying file LICENSE or https://www.apache.org/licenses/LICENSE-2.0

extern "C" {
#include <unistd.h>
#include <getopt.h>
#include <string.h>
}
#include <iostream>
#include <fstream>

#include "config.h"
#include "export.h"
#include "repl.h"
#include "shapes.h"
#include "livemode.h"
#include "version.h"

#include <libcurv/context.h>
#include <libcurv/die.h>
#include <libcurv/exception.h>
#include <libcurv/gpu_program.h>
#include <libcurv/output_file.h>
#include <libcurv/progdir.h>
#include <libcurv/program.h>
#include <libcurv/source.h>
#include <libcurv/system.h>

#include <libcurv/geom/builtin.h>
#include <libcurv/geom/import.h>
#include <libcurv/geom/tempfile.h>
#include <libcurv/viewer/viewer.h>

namespace fs = curv::Filesystem;

curv::System&
make_system(const char* argv0, std::list<const char*>& libs, std::ostream& out)
{
    static curv::System_Impl sys(out);
    if (isatty(2)) sys.use_colour_ = true;
    try {
        curv::geom::add_builtins(sys);
        curv::geom::add_importers(sys);
        if (argv0 != nullptr) {
            const char* CURV_LIBDIR = getenv("CURV_LIBDIR");
            namespace fs = boost::filesystem;
            curv::Shared<const curv::String> stdlib;
            if (CURV_LIBDIR != nullptr) {
                if (CURV_LIBDIR[0] != '\0') {
                    fs::path stdlib_path = fs::path(CURV_LIBDIR) / "std.curv";
                    stdlib = curv::make_string(stdlib_path.string().c_str());
                } else
                    stdlib = nullptr;
            } else {
                fs::path stdlib_path = fs::canonical(
                    curv::progdir(argv0) / "../lib/curv/std.curv");
                stdlib = curv::make_string(stdlib_path.string().c_str());
            }
            sys.load_library(stdlib);
        }
        for (const char* lib : libs) {
            sys.load_library(curv::make_string(lib));
        }
        return sys;
    } catch (std::exception& e) {
        sys.error(e);
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
"   Interactive command-line shell.\n"
"curv -l [-e] [options] filename\n"
"   Live programming mode. Evaluate & display result each time file changes.\n"
"   -e : Open editor window. $CURV_EDITOR overrides default editor.\n"
"curv [-o arg] [-x] [options] filename\n"
"   Batch mode. Evaluate file, display result or export to a file.\n"
"   -o format : Convert to specified file format, write data to stdout.\n"
;

const char help_infix[] =
"   -o filename.ext : Export to file, using filename extension as format.\n"
"   -x : Interpret filename argument as expression.\n"
"general options:\n"
"   -v : Verbose & debug output.\n"
"   -O name=value : Set parameter controlling the specified output format.\n"
"      If '-o fmt' is specified, use 'curv --help -o fmt' for help.\n"
"      If '-o fmt' is not specified, the following parameters are available:\n"
;

const char help_suffix[] =
"   $CURV_LIBDIR : Standard library directory, overrides PREFIX/lib/curv\n"
"   -n : Don't include standard library.\n"
"   -i file : Include specified library; may be repeated.\n"
;

int
main(int argc, char** argv)
{
    const char* argv0 = argv[0];

    // Parse arguments for general case.
    const char* usestdlib = argv0;
    fs::path opath;
    using ExPtr = decltype(exporters)::const_iterator;
    ExPtr exporter = exporters.end();
    Export_Params::Options options;
    bool verbose = false;
    bool live = false;
    std::list<const char*> libs;
    bool expr = false;
    const char* editor = nullptr;
    bool help = false;
    bool version = false;

    constexpr int HELP = 1000;
    constexpr int VERSION = 1001;
    static struct option longopts[] = {
        {"help",    no_argument, nullptr, HELP },
        {"version", no_argument, nullptr, VERSION },
        {nullptr,   0,           nullptr, 0 }
    };

    int opt;
    while ((opt = getopt_long(argc, argv, ":o:O:lni:xev", longopts, NULL)) != -1)
    {
        switch (opt) {
        case HELP:
            help = true;
            break;
        case VERSION:
            version = true;
            break;
        case 'o':
          {
            const char* oarg = optarg;
            opath = oarg;
            std::string ext_string = opath.extension().string();
            const char* ext = ext_string.c_str();
            const char* oname;
            if (ext[0] == '.')
                oname = &ext[1];
            else {
                oname = oarg;
                opath.clear();
            }
            exporter = exporters.find(oname);
            if (exporter == exporters.end()) {
                std::cerr << "-o: format '" << oname << "' not supported.\n"
                          << "Use " << argv0 << " --help for help.\n";
                return EXIT_FAILURE;
            }
            break;
          }
        case 'O':
          {
            char* eq = strchr(optarg, '=');
            if (eq == nullptr) {
                options[std::string(optarg)] = curv::make_string("");
            } else {
                *eq = '\0';
                options[std::string(optarg)] = curv::make_string(eq+1);
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
        case 'v':
            verbose = true;
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
        if (exporter != exporters.end()) {
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
        if (expr || live || (exporter != exporters.end() && !help)) {
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

    // Handle 'curv --help'.
    if (help) {
        if (exporter != exporters.end()) {
            std::cout << exporter->second.synopsis << "\n";
            exporter->second.describe_options(std::cout);
        } else {
            std::cout << help_prefix;
            for (auto& ex : exporters) {
                std::cout << "      " << ex.first << " : "
                          << ex.second.synopsis << "\n";
            }
            std::cout << help_infix;
            describe_viewer_options(std::cout, "      ");
            std::cout << help_suffix;
        }
        return EXIT_SUCCESS;
    }

    // Handle 'curv --version'.
    if (version) {
        print_version(std::cout);
        return EXIT_SUCCESS;
    }

    // Create system, a precondition for parsing -O parameters.
    // This can fail, so we do as much argument validation as possible
    // before this point.
    curv::System& sys(make_system(usestdlib, libs, std::cerr));
    atexit(curv::geom::remove_all_tempfiles);

    try {
        auto config = get_config(sys, curv::make_symbol(
            exporter == exporters.end() ? "viewer" : "export"));
        Export_Params oparams(std::move(options), config, sys);
        if (exporter != exporters.end())
            oparams.format_ = exporter->first;
        oparams.verbose_ = verbose;

        curv::viewer::Viewer_Config viewer_config;
        if (exporter == exporters.end())
            parse_viewer_config(oparams, viewer_config);

        // Finally, do stuff.
        if (filename == nullptr) {
            interactive_mode(sys, viewer_config);
            return EXIT_SUCCESS;
        }

        if (live) {
            return live_mode(sys, editor, filename, viewer_config);
        }

        // batch mode
        curv::Shared<curv::Source> source;
        if (expr) {
            source = curv::make<curv::String_Source>("", filename);
        } else {
            source = curv::make<curv::File_Source>(
                curv::make_string(filename), curv::At_System{sys});
        }

        curv::Program prog{std::move(source), sys};
        prog.compile();
        auto value = prog.eval();

        if (exporter != exporters.end()) {
            curv::Output_File ofile{sys};
            if (opath.empty())
                ofile.set_ostream(&std::cout);
            else
                ofile.set_path(opath);
            exporter->second.call(value, prog, oparams, ofile);
            ofile.commit();
        } else {
            curv::GPU_Program gpu_prog{prog};
            if (gpu_prog.recognize(value, viewer_config)) {
                print_shape(gpu_prog);
                curv::viewer::Viewer viewer(viewer_config);
                viewer.set_shape(std::move(gpu_prog.vshape_));
                viewer.run();
            } else {
                std::cout << value << "\n";
            }
        }
    } catch (std::exception& e) {
        sys.error(e);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
