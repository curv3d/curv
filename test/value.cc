#include <gtest/gtest.h>
#include <curv/value.h>
using namespace curv;

TEST(curv, value)
{
    Value v;

    v = Value();
    EXPECT_TRUE(v.is_null());

    v = Value(false);
    EXPECT_TRUE(v.is_bool());
    EXPECT_TRUE(v.get_bool_unsafe() == false);

    v = Value(true);
    EXPECT_TRUE(v.is_bool());
    EXPECT_TRUE(v.get_bool_unsafe() == true);

    v = Value(0.0);
    EXPECT_TRUE(v.is_num());
    EXPECT_TRUE(v.get_num_unsafe() == 0.0);

    v = Value(0.0/0.0);
    EXPECT_FALSE(v.is_num());
    EXPECT_TRUE(v.is_null());

    auto ptr = aux::make_shared<Ref_Value>(42);
    EXPECT_TRUE(ptr->use_count == 1);
    v = Value(ptr);
    ASSERT_TRUE(v.is_ref());
    EXPECT_TRUE(v.get_ref_unsafe().use_count == 2);
    EXPECT_TRUE(v.get_ref_unsafe().type_ == 42);
    ptr = nullptr;
    EXPECT_TRUE(v.get_ref_unsafe().use_count == 1);
}
