#include <gtest/gtest.h>
#undef FAIL

#include <libcurv/string.h>
#include <libcurv/symbol.h>
#include <sstream>
#include <iostream>
#include <cstdlib>
#include <cstring>
#include <cmath>

using namespace std;
using namespace curv;

TEST(curv, string)
{
    auto s0 = make_string("foo");
    ASSERT_EQ(s0->size(), 3u);
    ASSERT_TRUE(strcmp(s0->data(), "foo") == 0);
    ASSERT_STREQ(s0->c_str(), "foo");
    ASSERT_EQ(s0->type_, Ref_Value::ty_abstract_list);
    ASSERT_EQ(s0->subtype_, Ref_Value::sty_string);

    auto s1 = stringify("sqrt(2)==",sqrt(2));
    ASSERT_STREQ(s1->c_str(), "sqrt(2)==1.4142135623730951");

    Symbol_Ref a0 = make_symbol("foo");
    auto sym0 = a0.to_value().maybe<Symbol>();
    ASSERT_EQ(sym0->type_, Ref_Value::ty_symbol);
    ASSERT_EQ(sym0->subtype_, 0);
    Symbol_Ref a1 = make_symbol("foo");
    ASSERT_TRUE(a0 == a1);
    Symbol_Ref a2 = make_symbol("bar");
    ASSERT_FALSE(a0 == a2);
    ASSERT_TRUE(a2 < a0);
    ASSERT_FALSE(a0 < a2);
    ASSERT_FALSE(a0 < a0);

    Symbol_Ref anull;
    ASSERT_TRUE(anull.empty());
    ASSERT_FALSE(a0.empty());

    Symbol_Map<int> m;
    //m["0"] = 0;
    m[make_symbol("1")] = 1;
    //m["2"] = 2;
    //m["3"] = 3;
    //m["4"] = 4;
    //ASSERT_TRUE(m["0"] == 0);
    ASSERT_TRUE(m[make_symbol("1")] == 1);
    //ASSERT_TRUE(m["2"] == 2);
    //ASSERT_TRUE(m["3"] == 3);
    //ASSERT_TRUE(m["4"] == 4);
}
