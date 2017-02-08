// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

extern "C" {
#include <aux/readlinex.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
#include <unistd.h>
}
#include <iostream>

#include <aux/dtostr.h>
#include <aux/progdir.h>
#include <curv/analyzer.h>
#include <curv/context.h>
#include <curv/eval.h>
#include <curv/exception.h>
#include <curv/file.h>
#include <curv/parse.h>
#include <curv/phrase.h>
#include <curv/shared.h>
#include <curv/system.h>
#include <curv/list.h>
#include <curv/record.h>
#include <curv/gl_compiler.h>
#include <curv/shape.h>

bool was_interrupted = false;

void interrupt_handler(int)
{
    was_interrupted = true;
}

struct CString_Script : public curv::Script
{
    char* buffer_;

    CString_Script(const char* name, char* buffer)
    :
        curv::Script(curv::make_string(name), buffer, buffer + strlen(buffer)),
        buffer_(buffer)
    {}

    ~CString_Script()
    {
        free(buffer_);
    }
};

curv::System&
make_system(const char* argv0)
{
    try {
        const char* CURV_STDLIB = getenv("CURV_STDLIB");
        namespace fs = boost::filesystem;
        fs::path stdlib_path;
        if (CURV_STDLIB != nullptr)
            stdlib_path = CURV_STDLIB;
        else
            stdlib_path = aux::progdir(argv0) / "../lib/std.curv";
        static curv::System_Impl sys(
            curv::make_string(stdlib_path.c_str()),
            std::cerr);
        return sys;
    } catch (curv::Exception& e) {
        std::cerr << "ERROR: " << e << "\n";
        exit(EXIT_FAILURE);
    } catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}

int
interactive_mode(const char* argv0)
{
    // Catch keyboard interrupts, and set was_interrupted = true.
    // This is/will be used to interrupt the evaluator.
    struct sigaction interrupt_action;
    memset((void*)&interrupt_action, 0, sizeof(interrupt_action));
    interrupt_action.sa_handler = interrupt_handler;
    sigaction(SIGINT, &interrupt_action, nullptr);

    curv::System& sys(make_system(argv0));

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
            curv::Shared<curv::Module> module{eval_script(*script, names, sys)};
            for (auto f : *module)
                names[f.first] = curv::make<curv::Builtin_Value>(f.second);
            for (auto e : *module->elements())
                std::cout << e << "\n";
        } catch (curv::Exception& e) {
            std::cout << "ERROR: " << e << "\n";
        } catch (std::exception& e) {
            std::cout << "ERROR: " << e.what() << "\n";
        }
    }
}

void export_curv(curv::Module& module, std::ostream& out)
{
    for (auto e : *module.elements())
        out << e << "\n";
}
void export_shadertoy(curv::Module& module, std::ostream& out)
{
    std::vector<curv::Shared<curv::Shape2D>> shapes;
    for (auto e : *module.elements())
        if (auto shape = e.dycast<curv::Shape2D>())
            shapes.push_back(shape);
    // TODO: construct a union of `shapes`
    if (!shapes.empty())
        curv::gl_compile(*shapes.front(), std::cout);
}
bool is_json_data(curv::Value val)
{
    if (val.is_ref()) {
        auto& ref = val.get_ref_unsafe();
        switch (ref.type_) {
        case curv::Ref_Value::ty_string:
        case curv::Ref_Value::ty_list:
        case curv::Ref_Value::ty_record:
            return true;
        default:
            return false;
        }
    } else {
        return true; // null, bool or num
    }
}
bool export_json_value(curv::Value val, std::ostream& out)
{
    if (val.is_null()) {
        out << "null";
        return true;
    }
    if (val.is_bool()) {
        out << val;
        return true;
    }
    if (val.is_num()) {
        out << aux::dfmt(val.get_num_unsafe(), aux::dfmt::JSON);
        return true;
    }
    assert(val.is_ref());
    auto& ref = val.get_ref_unsafe();
    switch (ref.type_) {
    case curv::Ref_Value::ty_string:
      {
        auto& str = (curv::String&)ref;
        out << '"';
        for (auto c : str) {
            if (c == '\\' || c == '"')
                out << '\\';
            out << c;
        }
        out << '"';
        return true;
      }
    case curv::Ref_Value::ty_list:
      {
        auto& list = (curv::List&)ref;
        out << "[";
        bool first = true;
        for (auto e : list) {
            if (is_json_data(e)) {
                if (!first) out << ",";
                first = false;
                export_json_value(e, out);
            }
        }
        out << "]";
        return true;
      }
    case curv::Ref_Value::ty_record:
      {
        auto& record = (curv::Record&)ref;
        out << "{";
        bool first = true;
        for (auto i : record.fields_) {
            if (is_json_data(i.second)) {
                if (!first) out << ",";
                first = false;
                out << '"' << i.first << "\":";
                export_json_value(i.second, out);
            }
        }
        out << "}";
        return true;
      }
    default:
        return false;
    }
}
void export_json(curv::Module& module, std::ostream& out)
{
    for (auto e : *module.elements())
        if (export_json_value(e, out)) out << "\n";
}

const char help[] =
"Interactive CLI mode:\n"
"  curv\n"
"\n"
"Batch mode (process a file, write results to stdout):\n"
"  curv [options] filename\n"
"  -i script -- read definitions from <filename>, evaluate <script> as input\n"
"  -o format -- output format:\n"
"     curv -- default format, print values as Curv expressions\n"
"     json -- print values as JSON expressions\n"
"     shadertoy -- render shapes as a shadertoy.com image shader\n"
"  -D definition -- override an existing definition in <filename>\n"
"  filename -- input file, a Curv script, optional if -i specified\n"
"\n"
"Display version:\n"
"  curv --version\n"
"\n"
"Display this help information:\n"
"  curv --help\n"
;

int
main(int argc, char** argv)
{
    if (argc < 2)
        return interactive_mode(argv[0]);
    if (argc == 2 && strcmp(argv[1], "--help") == 0) {
        std::cout << help;
        return EXIT_SUCCESS;
    }
    if (argc == 2 && strcmp(argv[1], "--version") == 0) {
        std::cout << "Curv: initial development phase (no version yet)\n";
        return EXIT_SUCCESS;
    }

    // Parse batch mode arguments.
    const char* argv0 = argv[0];
    void (*exporter)(curv::Module&, std::ostream&) = export_curv;
    int opt;
    while ((opt = getopt(argc, argv, ":i:o:D:")) != -1) {
        switch (opt) {
        case 'o':
            if (strcmp(optarg, "curv") == 0)
                exporter = export_curv;
            else if (strcmp(optarg, "json") == 0)
                exporter = export_json;
            else if (strcmp(optarg, "shadertoy") == 0)
                exporter = export_shadertoy;
            else {
                std::cerr << "-o: format " << optarg << " not supported\n"
                          << "Use " << argv0 << " --help for help.\n";
                return EXIT_FAILURE;
            }
            break;
        case 'i':
        case 'D':
            std::cerr << "-" << (char)opt << " option not implemented yet\n";
            return EXIT_FAILURE;
        case '?':
            std::cerr << "-" << (char)optopt << ": unknown option\n"
                     << "Use " << argv0 << " --help for help.\n";
            return EXIT_FAILURE;
        case ':':
            std::cerr << "-" << (char)optopt << ": missing argument\n"
                     << "Use " << argv0 << " --help for help.\n";
            return EXIT_FAILURE;
        default:
            assert(0);
        }
    }
    if (optind >= argc) {
        std::cerr << "missing filename argument\n"
                  << "Use " << argv0 << " --help for help.\n";
        return EXIT_FAILURE;
    }
    if (argc - optind > 1) {
        std::cerr << "too many filename arguments\n"
                  << "Use " << argv0 << " --help for help.\n";
        return EXIT_FAILURE;
    }
    const char* filename = argv[optind];

    // Execute batch mode.
    curv::System& sys(make_system(argv0));
    try {
        auto file = curv::make<curv::File_Script>(
            curv::make_string(filename), curv::Context{});
        auto module = eval_script(*file, sys);
        exporter(*module, std::cout);
    } catch (curv::Exception& e) {
        std::cerr << "ERROR: " << e << "\n";
        return EXIT_FAILURE;
    } catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
