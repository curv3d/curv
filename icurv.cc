/*
 * Copyright 2016 Doug Moen. See LICENCE.md file for terms of use.
 */
extern "C" {
#include <readline/readline.h>
#include <string.h>
}
#include <iostream>
#include <curv/parse.h>
#include <curv/eval.h>
#include <curv/exception.h>

int
main(int, char**)
{
    for (;;) {
        char* line = readline("curv> ");
        if (line == nullptr) {
            std::cout << "\n";
            return 0;
        }
        curv::Script script("<stdin>", line, line + strlen(line));
        try {
            auto expr = curv::parse(script);
            double val = curv::eval(*expr);
            std::cout << val << "\n";
        } catch (curv::Exception& e) {
            std::cout << e << "\n";
        }
        free(line);
    }
}
