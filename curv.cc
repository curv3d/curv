// Copyright Doug Moen 2016.
// Distributed under The MIT License.
// See accompanying file LICENSE.md or https://opensource.org/licenses/MIT

extern "C" {
#include <aux/readlinex.h>
#include <string.h>
#include <signal.h>
#include <stdlib.h>
}
#include <iostream>

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
        curv::Script(curv::mk_string(name), buffer, buffer + strlen(buffer)),
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
        static curv::System_Impl sys(curv::mk_string(stdlib_path.c_str()).get());
        return sys;
    } catch (std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        exit(1);
    }
}

int
main(int argc, char** argv)
{
    curv::System& sys(make_system(argv[0]));

    if (argc > 2) {
        std::cerr << "too many arguments\n";
        exit(1);
    }
    if (argc == 2) {
        try {
            auto file = curv::make<curv::File_Script>(
                curv::mk_string(argv[1]), curv::Context{});
            auto module = eval_script(*file, sys);
            for (auto e : *module->elements())
                std::cout << e << "\n";
        } catch (curv::Exception& e) {
            std::cerr << "ERROR: " << e << "\n";
            exit(1);
        } catch (std::exception& e) {
            std::cerr << "ERROR: " << e.what() << "\n";
            exit(1);
        }
        exit(0);
    }

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
            return 0;
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
