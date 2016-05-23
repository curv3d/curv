#include <gtest/gtest.h>
#include <curv/value.h>
using namespace curv;

TEST(curv, value)
{
    Value v;

    v = k_void;
    EXPECT_TRUE(v.is_void());

    v = k_null;
    EXPECT_TRUE(v.is_null());

    v = k_false;
    EXPECT_TRUE(v.is_bool());
    EXPECT_TRUE(v.get_bool_unsafe() == false);

    v = mk_bool(true);
    EXPECT_TRUE(v.is_bool());
    EXPECT_TRUE(v.get_bool_unsafe() == true);

    v = mk_num(0.0);
    EXPECT_TRUE(v.is_num());
    EXPECT_TRUE(v.get_num_unsafe() == 0.0);

    v = mk_num(0.0/0.0);
    EXPECT_FALSE(v.is_num());
    EXPECT_TRUE(v.is_null());
}
