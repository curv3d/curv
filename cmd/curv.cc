// Copyright 2016-2018 Doug Moen
// Licensed under the Apache License, version 2.0
// See https://www.apache.org/licenses/LICENSE-2.0

extern "C" {
#include "readlinex.h"
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
#include <curv/dtostr.h>
#include <curv/analyser.h>
#include <curv/context.h>
#include <curv/program.h>
#include <curv/exception.h>
#include <curv/file.h>
#include <curv/parser.h>
#include <curv/phrase.h>
#include <curv/shared.h>
#include <curv/system.h>
#include <curv/list.h>
#include <curv/record.h>
#include <curv/gl_compiler.h>
#include <curv/shape.h>
#include <curv/version.h>
#include <curv/die.h>

bool was_interrupted = false;

void interrupt_handler(int)
{
    was_interrupted = true;
}

struct CString_Script : public curv::Script
{
    char* buffer_;

    // buffer argument is a static string.
    CString_Script(const char* name, const char* buffer)
    :
        curv::Script(curv::make_string(name), buffer, buffer + strlen(buffer)),
        buffer_(nullptr)
    {
    }

    // buffer argument is a heap string, allocated using malloc.
    CString_Script(const char* name, char* buffer)
    :
        curv::Script(curv::make_string(name), buffer, buffer + strlen(buffer)),
        buffer_(buffer)
    {}

    ~CString_Script()
    {
        if (buffer_) free(buffer_);
    }
};

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
                fs::path stdlib_path = progdir(argv0) / "../lib/std.curv";
                stdlib = curv::make_string(stdlib_path.c_str());
            }
            sys.load_library(stdlib);
        }
        for (const char* lib : libs) {
            sys.load_library(curv::make_string(lib));
        }
        return sys;
    } catch (curv::Exception& e) {
        std::cerr << "ERROR: " << e << "\n";
        exit(EXIT_FAILURE);
    } catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}

curv::Shared<curv::String> tempfile = nullptr;

curv::Shared<curv::String>
make_tempfile()
{
    auto filename = curv::stringify(",curv",getpid(),".frag");
    int fd = creat(filename->c_str(), 0666);
    if (fd == -1)
        throw curv::Exception({}, curv::stringify(
            "Can't create ",filename->c_str(),": ",strerror(errno)));
    close(fd);
    tempfile = filename;
    return filename;
}

void
remove_tempfile()
{
    if (tempfile != nullptr)
        remove(tempfile->c_str());
}

pid_t editor_pid = pid_t(-1);

void
launch_editor(const char* editor, const char* filename)
{
    pid_t pid = fork();
    if (pid == 0) {
        // in child process
        auto cmd = curv::stringify(editor, " ", filename);
        int r =
            execl("/bin/sh", "sh", "-c", cmd->c_str(), (char*)0);
        std::cerr << "can't exec $CURV_EDITOR\n"; // TODO: why?
        (void) r; // TODO
        exit(1);
    } else if (pid == pid_t(-1)) {
        std::cerr << "can't fork $CURV_EDITOR\n"; // TODO: why?
    } else {
        editor_pid = pid;
    }
}

bool
poll_editor()
{
    if (editor_pid == pid_t(-1))
        return false;
    else {
        int status;
        pid_t pid = waitpid(editor_pid, &status, WNOHANG);
        if (pid == editor_pid) {
            // TODO: print abnormal exit status
            editor_pid = pid_t(-1);
            return false;
        } else
            return true;
    }
}

pid_t viewer_pid = pid_t(-1);

void
poll_viewer()
{
    if (viewer_pid != pid_t(-1)) {
        int status;
        pid_t pid = waitpid(viewer_pid, &status, WNOHANG);
        if (pid == viewer_pid) {
            // TODO: print abnormal exit status
            viewer_pid = pid_t(-1);
        }
    }
}

void
launch_viewer(curv::Shared<curv::String> filename)
{
    poll_viewer();
    if (viewer_pid == pid_t(-1)) {
        pid_t pid = fork();
        if (pid == 0) {
            // in child process
            int r =
                execlp("glslViewer", "glslViewer", filename->c_str(), (char*)0);
            std::cerr << "can't exec glslViewer\n"; // TODO: why?
            (void) r; // TODO
            exit(1);
        } else if (pid == pid_t(-1)) {
            std::cerr << "can't fork glslViewer\n"; // TODO: why?
        } else {
            viewer_pid = pid;
        }
    }
}

bool
display_shape(curv::Value value, const curv::Context &cx, bool block = false)
{
    curv::Shape_Recognizer shape(cx);
    if (shape.recognize(value)) {
        auto filename = make_tempfile();
        std::ofstream f(filename->c_str());
        curv::gl_compile(shape, f, cx);
        f.close();
        if (block) {
            auto cmd = curv::stringify("glslViewer ",filename->c_str(),
                block ? "" : "&");
            system(cmd->c_str());
            unlink(filename->c_str());
        } else {
            launch_viewer(filename);
        }
        return true;
    } else
        return false;
}

int
interactive_mode(curv::System& sys)
{
    // Catch keyboard interrupts, and set was_interrupted = true.
    // This is/will be used to interrupt the evaluator.
    struct sigaction interrupt_action;
    memset((void*)&interrupt_action, 0, sizeof(interrupt_action));
    interrupt_action.sa_handler = interrupt_handler;
    sigaction(SIGINT, &interrupt_action, nullptr);

    // top level definitions, extended by typing 'id = expr'
    curv::Namespace names = sys.std_namespace();

    for (;;) {
        // Race condition on assignment to was_interrupted.
        was_interrupted = false;
        RLXResult result;
        char* line = readlinex("curv> ", &result);
        if (line == nullptr) {
            std::cout << "\n";
            if (result == rlx_interrupt) {
                continue;
            }
            return EXIT_SUCCESS;
        }
        auto script = curv::make<CString_Script>("", line);
        try {
            curv::Program prog{*script, sys};
            prog.compile(&names, nullptr);
            auto den = prog.denotes();
            if (den.first) {
                for (auto f : *den.first)
                    names[f.first] = curv::make<curv::Builtin_Value>(f.second);
            }
            if (den.second) {
                bool is_shape = false;
                if (den.second->size() == 1) {
                    static curv::Atom lastval_key = "_";
                    names[lastval_key] =
                        curv::make<curv::Builtin_Value>(den.second->front());
                    is_shape = display_shape(den.second->front(),
                        curv::At_Phrase(prog.value_phrase(), nullptr));
                }
                if (!is_shape) {
                    for (auto e : *den.second)
                        std::cout << e << "\n";
                }
            }
        } catch (curv::Exception& e) {
            std::cout << "ERROR: " << e << "\n";
        } catch (std::exception& e) {
            std::cout << "ERROR: " << e.what() << "\n";
        }
    }
}

int
live_mode(curv::System& sys, const char* editor, const char* filename)
{
    if (editor) {
        launch_editor(editor, filename);
        if (!poll_editor())
            return 1;
    }
    for (;;) {
        struct stat st;
        if (stat(filename, &st) != 0) {
            // file doesn't exist.
            memset((void*)&st, 0, sizeof(st));
        } else {
            // evaluate file.
            try {
                auto file = curv::make<curv::File_Script>(
                    curv::make_string(filename), curv::Context{});
                curv::Program prog{*file, sys};
                prog.compile();
                auto value = prog.eval();
                if (display_shape(value,
                    curv::At_Phrase(prog.value_phrase(), nullptr)))
                {
                    std::cout << "ok.\n";
                } else {
                    std::cout << value << "\n";
                }
            } catch (curv::Exception& e) {
                std::cout << "ERROR: " << e << "\n";
            } catch (std::exception& e) {
                std::cout << "ERROR: " << e.what() << "\n";
            }
        }
        // Wait for file to change or editor to quit.
        for (;;) {
            usleep(500'000);
            if (editor && !poll_editor()) {
                if (viewer_pid != (pid_t)(-1))
                    kill(viewer_pid, SIGTERM);
                return 0;
            }
            struct stat st2;
            if (stat(filename, &st2) != 0)
                memset((void*)&st2, 0, sizeof(st));
            if (st.st_mtime != st2.st_mtime)
                break;
        }
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
"   stl -- binary STL file (shape only)\n"
"   png -- PNG image file (shape only)\n"
"-O name=value -- parameter for one of the output formats\n"
"--version -- display version.\n"
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
        std::cout << "Curv " << curv::version << "\n";
        return EXIT_SUCCESS;
    }

    // Parse arguments.
    const char* argv0 = argv[0];
    void (*exporter)(curv::Value,
        curv::System&, const curv::Context&, const Export_Params&,
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
            else if (strcmp(optarg, "png") == 0)
                exporter = export_png;
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
                std::cerr << "-O argument must have form 'name=value'\n"
                          << "Use " << argv0 << " --help for help.\n";
                return EXIT_FAILURE;
            }
            *eq = '\0';
            eparams[std::string(optarg)] = std::string(eq+1);
            *eq = '=';
            break;
          }
        case 'l':
            live = true;
            break;
        case 'n':
            argv0 = nullptr;
            break;
        case 'i':
            libs.push_back(optarg);
            break;
        case 'x':
            expr = true;
            break;
        case 'e':
            editor = getenv("CURV_EDITOR");
            if (editor == nullptr) {
                std::cerr << "-e specified but $CURV_EDITOR not defined\n"
                         << "Use " << argv0 << " --help for help.\n";
                return EXIT_FAILURE;
            }
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
    curv::System& sys(make_system(argv0, libs));
    atexit(remove_tempfile);

    if (filename == nullptr) {
        return interactive_mode(sys);
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
            if (!display_shape(value,
                curv::At_Phrase(prog.value_phrase(), nullptr),
                true))
            {
                std::cout << value << "\n";
            }
        } else {
            exporter(value,
                sys,
                curv::At_Phrase(prog.value_phrase(), nullptr),
                eparams,
                std::cout);
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
