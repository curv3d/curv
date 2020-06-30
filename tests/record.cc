#include <gtest/gtest.h>
#undef FAIL
#include <libcurv/context.h>
#include <libcurv/record.h>
#include <libcurv/value.h>
#include <sstream>
#include <iostream>
#include "sys.h"
using namespace curv;

static bool prints_as(Value val, const char* expect)
{
    std::stringstream ss;
    val.print_repr(ss);
    if (ss.str() == expect)
        return true;
    else {
        std::cout << "expected '" << expect << "' got '" << ss.str() << "'\n";
        return false;
    }
}

TEST(curv, record)
{
    At_System cx{sys};

    auto r = make<DRecord>();
    r->fields_[make_symbol("a")] = Value{1.0};
    r->fields_[make_symbol("b")] = Value{true};
    ASSERT_TRUE(prints_as(Value{r}, "{a:1,b:#true}"));
    auto i = r->iter();

    // at first field
    ASSERT_FALSE(i->empty());
    ASSERT_EQ(i->key(), make_symbol("a"));
    ASSERT_TRUE(i->value(cx).equal({1.0},cx).to_bool());
    i->next();

    // at second field
    ASSERT_FALSE(i->empty());
    ASSERT_EQ(i->key(), make_symbol("b"));
    ASSERT_TRUE(i->value(cx).equal({true},cx).to_bool());
    i->next();

    // at end
    ASSERT_TRUE(i->empty());
}
