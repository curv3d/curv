#include <gtest/gtest.h>
#include <libcurv/list.h>

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
    ASSERT_TRUE((*x)[0].eq(Value{42.0}));
    ASSERT_EQ(x->use_count, 1u);
    ASSERT_EQ(y->use_count, 2u);
    x = nullptr;
    ASSERT_EQ(y->use_count, 1u);
}
