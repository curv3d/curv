#include <gtest/gtest.h>
#include <curv/string.h>
#include <curv/atom.h>
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
    auto s0 = make_string("foo");
    ASSERT_EQ(s0->size(), 3);
    ASSERT_TRUE(strcmp(s0->data(), "foo") == 0);
    ASSERT_STREQ(s0->c_str(), "foo");

    auto s1 = stringify("sqrt(2)==",sqrt(2));
    ASSERT_STREQ(s1->c_str(), "sqrt(2)==1.4142135623730951");

    Atom a0("foo");
    Atom a1("foo");
    ASSERT_TRUE(a0 == a1);
    Atom a2("bar");
    ASSERT_FALSE(a0 == a2);
    ASSERT_TRUE(a2 < a0);
    ASSERT_FALSE(a0 < a2);
    ASSERT_FALSE(a0 < a0);
    Atom a3("barf", 3);
    ASSERT_TRUE(a2 == a3);

    Atom_Map<int> m;
    //m["0"] = 0;
    m["1"] = 1;
    //m["2"] = 2;
    //m["3"] = 3;
    //m["4"] = 4;
    //ASSERT_TRUE(m["0"] == 0);
    ASSERT_TRUE(m["1"] == 1);
    //ASSERT_TRUE(m["2"] == 2);
    //ASSERT_TRUE(m["3"] == 3);
    //ASSERT_TRUE(m["4"] == 4);
}
