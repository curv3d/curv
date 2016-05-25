// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

extern "C" {
#include <aux/readlinex.h>
#include <string.h>
#include <signal.h>
}
#include <iostream>
#include <curv/parse.h>
#include <curv/eval.h>
#include <curv/exception.h>
#include <curv/syntax.h>

bool was_interrupted = false;

void interrupt_handler(int)
{
    was_interrupted = true;
}

int
main(int, char**)
{
    // Catch keyboard interrupts, and set was_interrupted = true.
    // This is/will be used to interrupt the evaluator.
    struct sigaction interrupt_action;
    memset((void*)&interrupt_action, 0, sizeof(interrupt_action));
    interrupt_action.sa_handler = interrupt_handler;
    sigaction(SIGINT, &interrupt_action, nullptr);

    // top level definitions, extended by typing 'id = expr'
    curv::Namespace names = curv::builtin_namespace;

    for (;;) {
        // Race condition on assignment to was_interrupted.
        was_interrupted = false;
        RLXResult result;
        char* line = readlinex("tcad> ", &result);
        if (line == nullptr) {
            std::cout << "\n";
            if (result == rlx_interrupt) {
                continue;
            }
            return 0;
        }
        curv::Script script("<stdin>", line, line + strlen(line));
        try {
            auto syntax = curv::parse(script);
            if (syntax == nullptr) // blank line
                continue;
            const curv::Definition* def =
                dynamic_cast<curv::Definition*>(syntax.get());
            if (def != nullptr) {
                const curv::Identifier* id =
                    dynamic_cast<curv::Identifier*>(def->left_.get());
                if (id == nullptr) {
                    throw curv::SyntaxError(script, def->equate_,
                        "= not preceded by identifier");
                }
                curv::Value val = curv::eval(*def->right_, names);
                names[id->id_.range(script)] = val;
            } else {
                curv::Value val = curv::eval(*syntax, names);
                val.print(std::cout);
                std::cout << "\n";
            }
        } catch (curv::Exception& e) {
            std::cout << e << "\n";
        }
        free(line);
    }
}
