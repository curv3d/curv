extern "C" {
#include <readline/readline.h>
#include <string.h>
}
#include <iostream>
#include <curv/parse.h>
#include <curv/eval.h>

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
        auto expr = curv::parse(script);
        double val = curv::eval(*expr);
        std::cout << val << "\n";
        free(line);
    }
}
