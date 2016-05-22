// Copyright Doug Moen 2016.
// Distributed under the Boost Software License, Version 1.0.
// See accompanying file LICENCE.md or http://www.boost.org/LICENSE_1_0.txt

extern "C" {
#include <curv/readlinex.h>
#include <string.h>
#include <signal.h>
}
#include <iostream>
#include <curv/parse.h>
#include <curv/eval.h>
#include <curv/exception.h>

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
            auto expr = curv::parse(script);
            double val = curv::eval(*expr, curv::builtin_namespace);
            std::cout << val << "\n";
        } catch (curv::Exception& e) {
            std::cout << e << "\n";
        }
        free(line);
    }
}
