#include <gtest/gtest.h>
#include <libcurv/list.h>
//#include <libcurv/symbol.h>
//#include <sstream>
//#include <iostream>
//#include <cstdlib>
//#include <cstring>
//#include <cmath>

using namespace std;
using namespace curv;

TEST(curv, list)
{
    auto xp = List::make(2);
    auto x = Shared<List>{std::move(xp)};

    auto yp = List::make(2);
    auto y = Shared<List>{std::move(yp)};

    (*x)[0] = Value{42.0};
    (*x)[1] = Value{y};

    ASSERT_EQ(x->size(), 2u);
    ASSERT_EQ((*x)[0], Value{42.0});
    ASSERT_EQ(x->use_count, 1u);
    ASSERT_EQ(y->use_count, 2u);
    x = nullptr;
    ASSERT_EQ(y->use_count, 1u);

/*
    auto s0 = make_string("foo");
    ASSERT_EQ(s0->size(), 3);
    ASSERT_TRUE(strcmp(s0->data(), "foo") == 0);
    ASSERT_STREQ(s0->c_str(), "foo");

    auto s1 = stringify("sqrt(2)==",sqrt(2));
    ASSERT_STREQ(s1->c_str(), "sqrt(2)==1.4142135623730951");

    Symbol a0("foo");
    Symbol a1("foo");
    ASSERT_TRUE(a0 == a1);
    Symbol a2("bar");
    ASSERT_FALSE(a0 == a2);
    ASSERT_TRUE(a2 < a0);
    ASSERT_FALSE(a0 < a2);
    ASSERT_FALSE(a0 < a0);
    Symbol a3("barf", 3);
    ASSERT_TRUE(a2 == a3);

    Symbol_Map<int> m;
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
*/
}
