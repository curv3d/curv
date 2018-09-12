#include <gtest/gtest.h>
#include <libcurv/context.h>
#include <libcurv/record.h>
#include <libcurv/value.h>
#include <sstream>
#include <iostream>
using namespace curv;

static bool prints_as(Value val, const char* expect)
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

TEST(curv, record)
{
    auto r = make<DRecord>();
    r->fields_[Symbol{"a"}] = Value{1.0};
    r->fields_[Symbol{"b"}] = Value{true};
    ASSERT_TRUE(prints_as(Value{r}, "{a:1,b:true}"));
    auto i = r->iter();

    // at first field
    ASSERT_FALSE(i->empty());
    ASSERT_EQ(i->key(), Symbol{"a"});
    ASSERT_TRUE(i->value({}).equal({1.0},{}));
    i->next();

    // at second field
    ASSERT_FALSE(i->empty());
    ASSERT_EQ(i->key(), Symbol{"b"});
    ASSERT_TRUE(i->value({}).equal({true},{}));
    i->next();

    // at end
    ASSERT_TRUE(i->empty());
}
