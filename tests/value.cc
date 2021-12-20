#include <gtest/gtest.h>
#undef FAIL
#include <libcurv/value.h>
#include <libcurv/frame.h>
#include <libcurv/function.h>
#include <libcurv/string.h>
#include <libcurv/context.h>
#include "sys.h"
#include <sstream>
#include <iostream>
using namespace curv;

bool prints_as(Value val, const char* expect)
{
    std::stringstream ss;
    val.print_repr(ss, Prec::item);
    if (ss.str() == expect)
        return true;
    else {
        std::cout << "expected '" << expect << "' got '" << ss.str() << "'\n";
        return false;
    }
}


struct Id_Function : public Tuple_Function
{
    Id_Function() : Tuple_Function(1,"id") {}
    Value tuple_call(Fail, Frame& args) const override
    {
        return args[0];
    }
};

TEST(curv, value)
{
    Value v;
    At_System cx(sys);

    v = Value();
    EXPECT_TRUE(v.is_missing());
    EXPECT_FALSE(v.is_bool());
    EXPECT_FALSE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_TRUE(prints_as(v, "<missing>"));

    v = Value(false);
    EXPECT_FALSE(v.is_missing());
    EXPECT_TRUE(v.is_bool());
    EXPECT_FALSE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_TRUE(v.to_bool_unsafe() == false);
    EXPECT_TRUE(v.eq(false));
    EXPECT_TRUE(prints_as(v, "#false"));

    v = Value(true);
    EXPECT_FALSE(v.is_missing());
    EXPECT_TRUE(v.is_bool());
    EXPECT_FALSE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_TRUE(v.to_bool_unsafe() == true);
    EXPECT_TRUE(prints_as(v, "#true"));

    v = Value(0.0);
    EXPECT_FALSE(v.is_missing());
    EXPECT_FALSE(v.is_bool());
    EXPECT_TRUE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_TRUE(v.to_num_unsafe() == 0.0);
    EXPECT_TRUE(prints_as(v, "0"));

    v = Value(-0.0);
    EXPECT_FALSE(v.is_missing());
    EXPECT_FALSE(v.is_bool());
    EXPECT_TRUE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_TRUE(v.to_num_unsafe() == 0.0);
    EXPECT_TRUE(prints_as(v, "-0"));

    v = Value(1.0/0.0);
    EXPECT_FALSE(v.is_missing());
    EXPECT_FALSE(v.is_bool());
    EXPECT_TRUE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_TRUE(v.to_num_unsafe() == 1.0/0.0);
    EXPECT_TRUE(prints_as(v, "inf"));

    v = Value(-1.0/0.0);
    EXPECT_FALSE(v.is_missing());
    EXPECT_FALSE(v.is_bool());
    EXPECT_TRUE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_EQ(v.to_num_unsafe(), -1.0/0.0);
    EXPECT_TRUE(prints_as(v, "-inf"));

    v = Value(0.0/0.0);
    EXPECT_TRUE(v.is_missing());
    EXPECT_FALSE(v.is_bool());
    EXPECT_FALSE(v.is_num());
    EXPECT_FALSE(v.is_ref());
    EXPECT_TRUE(prints_as(v, "<missing>"));

    auto ptr = make_string("abc", 3);
    EXPECT_TRUE(ptr->use_count == 1);
    v = Value(ptr);
    EXPECT_FALSE(v.is_missing());
    EXPECT_FALSE(v.is_bool());
    EXPECT_FALSE(v.is_num());
    ASSERT_TRUE(v.is_ref());
    EXPECT_TRUE(v.to_ref_unsafe().use_count == 2);
    EXPECT_TRUE(v.to_ref_unsafe().type_ == Ref_Value::ty_abstract_list);
    EXPECT_TRUE(v.to_ref_unsafe().subtype_ == Ref_Value::sty_string);
    ptr = nullptr;
    EXPECT_TRUE(v.to_ref_unsafe().use_count == 1);

    Generic_List glist(v, Fail::soft, cx);
    EXPECT_TRUE(glist.is_list());
    EXPECT_TRUE(glist.size() == 3);

#if 0
    v = make_ref_value<Ref_Value>(17);
    EXPECT_FALSE(v.is_missing());
    EXPECT_FALSE(v.is_bool());
    EXPECT_FALSE(v.is_num());
    ASSERT_TRUE(v.is_ref());
    EXPECT_TRUE(v.to_ref_unsafe().use_count == 1);
    EXPECT_TRUE(v.to_ref_unsafe().type_ == 17);
#endif

    v = make_ref_value<Id_Function>();
    EXPECT_FALSE(v.is_missing());
    EXPECT_FALSE(v.is_bool());
    EXPECT_FALSE(v.is_num());
    ASSERT_TRUE(v.is_ref());
    EXPECT_TRUE(v.to_ref_unsafe().use_count == 1);
    EXPECT_TRUE(v.to_ref_unsafe().type_ == Ref_Value::ty_function);
    Tuple_Function* f = (Tuple_Function*)&v.to_ref_unsafe();
    EXPECT_TRUE(f->use_count == 1);
    EXPECT_TRUE(f->type_ == Ref_Value::ty_function);
    EXPECT_TRUE(f->nargs_ == 1);

    // copy/move constructors
    {
        Value v0(make<Id_Function>());
        ASSERT_TRUE(v0.is_ref());
        Ref_Value& r0(v0.to_ref_unsafe());
        ASSERT_TRUE(r0.use_count == 1);
        ASSERT_TRUE(r0.type_ == Ref_Value::ty_function);

        Value v1(v0);
        ASSERT_TRUE(v1.is_ref());
        Ref_Value& r1(v1.to_ref_unsafe());
        ASSERT_TRUE(&r1 == &r0);
        ASSERT_TRUE(r1.use_count == 2);
        ASSERT_TRUE(r1.type_ == Ref_Value::ty_function);

        Value v2(std::move(v1));
        ASSERT_TRUE(v1.is_missing());
        ASSERT_TRUE(v2.is_ref());
        Ref_Value& r2(v2.to_ref_unsafe());
        ASSERT_TRUE(&r2 == &r0);
        ASSERT_TRUE(r2.use_count == 2);
        ASSERT_TRUE(r2.type_ == Ref_Value::ty_function);
    }
}
