#include <gtest/gtest.h>
#include <curv/string.h>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>

using namespace std;
using namespace aux;
using namespace curv;

TEST(curv, string)
{
    auto s0 = mk_string("foo");
    ASSERT_EQ(s0->size(), 3);
    ASSERT_TRUE(strcmp(s0->data(), "foo") == 0);
    ASSERT_STREQ(s0->c_str(), "foo");

    auto s1 = to_string("sqrt(2)==",sqrt(2));
    ASSERT_STREQ(s1->c_str(), "sqrt(2)==1.4142135623730951");
}
