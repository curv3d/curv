#if 0
#include <gtest/gtest.h>
#include <cstdlib>
#include <string>
#include <sstream>
#include <iostream>
#include <boost/process.hpp>

std::string curvout(std::string args)
{
    namespace bp = boost::process;
    bp::ipstream out;
    std::stringstream result;
    bp::child c("../debug/curv "+args, bp::std_out > out);
    result << out.rdbuf();
    c.wait();
    return result.str();
}
std::string curverr(std::string args)
{
    namespace bp = boost::process;
    bp::ipstream err;
    std::stringstream result;
    bp::child c("../debug/curv "+args, bp::std_err > err);
    result << err.rdbuf();
    c.wait();
    return result.str();
}

TEST(curv, cli)
{
    EXPECT_EQ(curverr("-O aa -x 42"),
        "ERROR: missing argument\n"
        "1| -O aa=\n"
        "         ^\n");
    EXPECT_EQ(curverr("-O foo -x 42"),
        "ERROR: 'foo': Unknown -O parameter.\n"
        "Use 'curv --help' for help.\n"
        "1| -O foo=\n"
        "      ^^^ \n");
    EXPECT_EQ(curverr("-O fdur=true -x 42"),
        "ERROR: true is not a number\n"
        "1| -O fdur=true\n"
        "           ^^^^\n");
}
#endif
