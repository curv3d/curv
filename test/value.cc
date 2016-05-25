#include <gtest/gtest.h>
#include <curv/value.h>
#include <sstream>
#include <iostream>
using namespace curv;

bool prints_as(Value val, const char* expect)
{
    std::stringstream ss;
    val.print(ss);
    if (ss.str() == expect)
        return true;
    else {
        std::cout << "expected '" << expect << "' got '" << ss.str() << "'\n";
        return false;
    }
}

TEST(curv, value)
{
    Value v;

    v = Value();
    EXPECT_TRUE(v.is_null());
    EXPECT_FALSE(v.is_bool());
    EXPECT_FALSE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_TRUE(prints_as(v, "null"));

    v = Value(false);
    EXPECT_FALSE(v.is_null());
    EXPECT_TRUE(v.is_bool());
    EXPECT_FALSE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_TRUE(v.get_bool_unsafe() == false);
    EXPECT_TRUE(prints_as(v, "false"));

    v = Value(true);
    EXPECT_FALSE(v.is_null());
    EXPECT_TRUE(v.is_bool());
    EXPECT_FALSE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_TRUE(v.get_bool_unsafe() == true);
    EXPECT_TRUE(prints_as(v, "true"));

    v = Value(0.0);
    EXPECT_FALSE(v.is_null());
    EXPECT_FALSE(v.is_bool());
    EXPECT_TRUE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_TRUE(v.get_num_unsafe() == 0.0);
    EXPECT_TRUE(prints_as(v, "0"));

    v = Value(1.0/0.0);
    EXPECT_FALSE(v.is_null());
    EXPECT_FALSE(v.is_bool());
    EXPECT_TRUE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_TRUE(v.get_num_unsafe() == 1.0/0.0);
    EXPECT_TRUE(prints_as(v, "inf"));

    v = Value(0.0/0.0);
    EXPECT_TRUE(v.is_null());
    EXPECT_FALSE(v.is_bool());
    EXPECT_FALSE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_TRUE(prints_as(v, "null"));

    auto ptr = aux::make_shared<Ref_Value>(42);
    EXPECT_TRUE(ptr->use_count == 1);
    v = Value(ptr);
    EXPECT_FALSE(v.is_null());
    EXPECT_FALSE(v.is_bool());
    EXPECT_FALSE(v.is_num());
    ASSERT_TRUE(v.is_ref());
    EXPECT_TRUE(v.get_ref_unsafe().use_count == 2);
    EXPECT_TRUE(v.get_ref_unsafe().type_ == 42);
    ptr = nullptr;
    EXPECT_TRUE(v.get_ref_unsafe().use_count == 1);
}
