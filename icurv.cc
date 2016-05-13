#include <iostream>
#include <curv/parse.h>
#include <curv/eval.h>

int
main(int, char**)
{
    std::string line;
    for (;;) {
        std::cout << "curv> ";
        if (!std::getline(std::cin, line)) {
            std::cout << "\n";
            return 0;
        }
        curv::Script script("<stdin>", line.data(), line.data() + line.size());
        auto expr = curv::parse(script);
        double val = curv::eval(*expr);
        std::cout << val << "\n";
    }
}
