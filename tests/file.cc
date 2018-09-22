#include <gtest/gtest.h>
#include <libcurv/output_file.h>
#include <sstream>
#include <fstream>
#include <cstdio>
#include "sys.h"

using namespace std;
using namespace curv;
namespace fs = Filesystem;

void writefile(const fs::path& path, const std::string& data)
{
    ofstream o(path.c_str());
    o << data;
    o.close();
}
std::string readfile(const fs::path& path)
{
    ifstream i(path.c_str());
    stringstream s;
    s << i.rdbuf();
    return s.str();
}

TEST(curv, output_file)
{
    Output_File f1{sys};
    stringstream b1;
    f1.set_ostream(&b1);
    f1.open();
    f1.ostream() << "foo";
    ASSERT_EQ(b1.str(), "");
    f1.commit();
    ASSERT_EQ(b1.str(), "foo");

    Output_File f2{sys};
    stringstream b2;
    f2.set_ostream(&b2);
    writefile(f2.path(), "foo");
    ASSERT_EQ(b2.str(), "");
    f2.commit();
    ASSERT_EQ(b2.str(), "foo");

    Output_File f3{sys};
    fs::path p3(",f3");
    f3.set_path(p3);
    f3.open();
    f3.ostream() << "foo";
    ASSERT_EQ(readfile(p3), "");
    f3.commit();
    ASSERT_EQ(readfile(p3), "foo");
    remove(",f3");

    Output_File f4{sys};
    fs::path p4(",f4");
    f4.set_path(p4);
    writefile(f4.path(), "foo");
    ASSERT_EQ(readfile(p4), "");
    f4.commit();
    ASSERT_EQ(readfile(p4), "foo");
    remove(",f4");
}
